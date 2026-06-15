#include "sqlite_api.h"
#include "sqlite3.h"
#include <iostream>
#include <vector>

std::shared_ptr<JSObject> createSqliteModule(Evaluator* evaluator) {
    auto sqliteModule = std::make_shared<JSObject>();
    sqliteModule->prototype = evaluator->objectPrototype;
    
    auto dbConstructor = std::make_shared<JSNativeFunction>("Database", [evaluator](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
        if (args.empty()) throw RuntimeError("Database requires a filename");
        std::string filename = args[0]->toString();
        
        sqlite3* db = nullptr;
        if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK) {
            std::string err = sqlite3_errmsg(db);
            sqlite3_close(db);
            throw RuntimeError("SQLite Error: " + err);
        }
        
        // We need to manage the DB lifetime.
        // For simplicity, we just leak it or close it when JS explicitly calls .close() 
        // A better approach is using a custom C++ struct that wraps it, but JS objects in this engine are JSValue.
        // We will just store the pointer address as a property!
        
        auto dbObj = std::make_shared<JSObject>();
        dbObj->prototype = evaluator->objectPrototype;
        
        dbObj->properties["_dbPtr"] = JSPropertyDescriptor{std::make_shared<JSNumber>(reinterpret_cast<uintptr_t>(db)), nullptr, nullptr, false, false, false};
        
        dbObj->properties["exec"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("exec", [dbObj](const std::vector<std::shared_ptr<JSValue>>& execArgs) -> std::shared_ptr<JSValue> {
            if (execArgs.empty()) throw RuntimeError("exec requires a SQL string");
            std::string sql = execArgs[0]->toString();
            
            sqlite3* db = reinterpret_cast<sqlite3*>(static_cast<uintptr_t>(dbObj->properties["_dbPtr"].value->toNumber()));
            char* zErrMsg = 0;
            int rc = sqlite3_exec(db, sql.c_str(), nullptr, 0, &zErrMsg);
            if (rc != SQLITE_OK) {
                std::string errStr = zErrMsg;
                sqlite3_free(zErrMsg);
                throw RuntimeError("SQLite Error: " + errStr);
            }
            return std::make_shared<JSUndefined>();
        }), nullptr, nullptr, true, true, true};
        
        dbObj->properties["query"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("query", [evaluator, dbObj](const std::vector<std::shared_ptr<JSValue>>& queryArgs) -> std::shared_ptr<JSValue> {
            if (queryArgs.empty()) throw RuntimeError("query requires a SQL string");
            std::string sql = queryArgs[0]->toString();
            
            sqlite3* db = reinterpret_cast<sqlite3*>(static_cast<uintptr_t>(dbObj->properties["_dbPtr"].value->toNumber()));
            sqlite3_stmt* stmt;
            
            int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
            if (rc != SQLITE_OK) {
                throw RuntimeError("SQLite Error: " + std::string(sqlite3_errmsg(db)));
            }
            
            auto rowsArray = std::make_shared<JSArray>();
            int rowIdx = 0;
            
            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                auto rowObj = std::make_shared<JSObject>();
                rowObj->prototype = evaluator->objectPrototype;
                
                int cols = sqlite3_column_count(stmt);
                for (int i = 0; i < cols; i++) {
                    std::string colName = sqlite3_column_name(stmt, i);
                    int type = sqlite3_column_type(stmt, i);
                    std::shared_ptr<JSValue> val;
                    
                    if (type == SQLITE_INTEGER) val = std::make_shared<JSNumber>(sqlite3_column_int64(stmt, i));
                    else if (type == SQLITE_FLOAT) val = std::make_shared<JSNumber>(sqlite3_column_double(stmt, i));
                    else if (type == SQLITE_TEXT) val = std::make_shared<JSString>(reinterpret_cast<const char*>(sqlite3_column_text(stmt, i)));
                    else val = std::make_shared<JSNull>();
                    
                    rowObj->properties[colName] = JSPropertyDescriptor{val, nullptr, nullptr, true, true, true};
                }
                
                rowsArray->properties[std::to_string(rowIdx++)] = JSPropertyDescriptor{rowObj, nullptr, nullptr, true, true, true};
            }
            rowsArray->properties["length"] = JSPropertyDescriptor{std::make_shared<JSNumber>(rowIdx), nullptr, nullptr, true, true, true};
            
            sqlite3_finalize(stmt);
            
            if (rc != SQLITE_DONE) {
                throw RuntimeError("SQLite Error during execution: " + std::string(sqlite3_errmsg(db)));
            }
            
            return rowsArray;
        }), nullptr, nullptr, true, true, true};
        
        dbObj->properties["close"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("close", [dbObj](const std::vector<std::shared_ptr<JSValue>>&) -> std::shared_ptr<JSValue> {
            sqlite3* db = reinterpret_cast<sqlite3*>(static_cast<uintptr_t>(dbObj->properties["_dbPtr"].value->toNumber()));
            if (db) sqlite3_close(db);
            dbObj->properties["_dbPtr"].value = std::make_shared<JSNumber>(0);
            return std::make_shared<JSUndefined>();
        }), nullptr, nullptr, true, true, true};
        
        return dbObj;
    });
    
    sqliteModule->properties["Database"] = JSPropertyDescriptor{dbConstructor, nullptr, nullptr, true, true, true};
    
    return sqliteModule;
}
