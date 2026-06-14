#include "evaluator.h"
#include <fstream>
#include "builtins.h"
#include <cmath>
#include <chrono>
#include <regex>
#include <cstdint>
#include "lexer.h"
#include "parser.h"

// --- Builtins ---
std::shared_ptr<JSValue> builtin_console_log(const std::vector<std::shared_ptr<JSValue>>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        std::cout << args[i]->toString();
        if (i < args.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    return std::make_shared<JSUndefined>();
}

void Evaluator::hoist(std::shared_ptr<Statement> stmt) {
    if (!stmt) return;
    if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(stmt)) {
        if (varDecl->isVar) {
            environment->defineVar(varDecl->name, std::make_shared<JSUndefined>());
        }
    } else if (auto destDecl = std::dynamic_pointer_cast<DestructuringDeclaration>(stmt)) {
        if (destDecl->isVar) {
            for (const auto& n : destDecl->names) {
                environment->defineVar(n, std::make_shared<JSUndefined>());
            }
        }
    } else if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(stmt)) {
        auto function = std::make_shared<JSFunction>(funcDecl, environment);
        environment->defineVar(funcDecl->name, function);
    } else if (auto classDecl = std::dynamic_pointer_cast<ClassDeclaration>(stmt)) {
        // Classes are not hoisted in the same way, they stay in temporal dead zone until defined.
    } else if (auto blockStmt = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
        for (auto s : blockStmt->statements) hoist(s);
    } else if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
        hoist(ifStmt->consequent);
        if (ifStmt->alternate) hoist(ifStmt->alternate);
    } else if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
        hoist(whileStmt->body);
    } else if (auto forStmt = std::dynamic_pointer_cast<ForStatement>(stmt)) {
        if (forStmt->init) hoist(forStmt->init);
        hoist(forStmt->body);
    } else if (auto tryStmt = std::dynamic_pointer_cast<TryStatement>(stmt)) {
        hoist(tryStmt->block);
        if (tryStmt->handler) hoist(tryStmt->handler);
        if (tryStmt->finalizer) hoist(tryStmt->finalizer);
    }
}

std::shared_ptr<JSValue> builtin_math_floor(const std::vector<std::shared_ptr<JSValue>>& args) {
    if (args.empty()) return std::make_shared<JSUndefined>();
    return std::make_shared<JSNumber>(std::floor(args[0]->toNumber()));
}

Evaluator::Evaluator() {
    environment = std::make_shared<Environment>();
    setupGlobalEnvironment();
}

void Evaluator::setupGlobalEnvironment() {
    objectPrototype = std::make_shared<JSObject>();
    objectPrototype->properties["hasOwnProperty"].value = std::make_shared<JSNativeFunction>("hasOwnProperty", [this](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSBoolean>(false));
        if (this->lastThisContext && this->lastThisContext->getType() == JSValueType::OBJECT) {
            auto obj = std::dynamic_pointer_cast<JSObject>(this->lastThisContext);
            return std::shared_ptr<JSValue>(std::make_shared<JSBoolean>(obj->properties.count(args[0]->toString()) > 0));
        }
        return std::shared_ptr<JSValue>(std::make_shared<JSBoolean>(false));
    });
    objectPrototype->properties["toString"].value = std::make_shared<JSNativeFunction>("toString", [this](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (this->lastThisContext && this->lastThisContext->getType() == JSValueType::OBJECT) {
            return std::shared_ptr<JSValue>(std::make_shared<JSString>("[object Object]"));
        }
        return std::shared_ptr<JSValue>(std::make_shared<JSString>("undefined"));
    });

    auto objectConstructor = std::make_shared<JSObject>();
    objectConstructor->prototype = objectPrototype;
    objectConstructor->properties["prototype"].value = objectPrototype;
    objectConstructor->properties["defineProperty"].value = std::make_shared<JSNativeFunction>("defineProperty", [this](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.size() < 3) throw RuntimeError("TypeError: Object.defineProperty requires 3 arguments");
        if (args[0]->getType() != JSValueType::OBJECT && args[0]->getType() != JSValueType::FUNCTION) throw RuntimeError("TypeError: Object.defineProperty called on non-object");
        auto obj = std::dynamic_pointer_cast<JSObject>(args[0]);
        std::string prop = args[1]->toString();
        auto desc = args[2];
        if (desc->getType() != JSValueType::OBJECT) throw RuntimeError("TypeError: Property description must be an object");
        auto descObj = std::dynamic_pointer_cast<JSObject>(desc);
        
        JSPropertyDescriptor pd;
        if (descObj->properties.count("value")) pd.value = descObj->properties["value"].value;
        if (descObj->properties.count("get")) pd.get = descObj->properties["get"].value;
        if (descObj->properties.count("set")) pd.set = descObj->properties["set"].value;
        if (descObj->properties.count("writable")) pd.writable = descObj->properties["writable"].value->isTruthy();
        if (descObj->properties.count("enumerable")) pd.enumerable = descObj->properties["enumerable"].value->isTruthy();
        if (descObj->properties.count("configurable")) pd.configurable = descObj->properties["configurable"].value->isTruthy();
        
        obj->properties[prop] = pd;
        return args[0];
    });
    environment->define("Object", objectConstructor);    
    // Add require
    environment->define("require", std::make_shared<JSNativeFunction>("require", [this](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
        if (args.empty()) throw RuntimeError("TypeError: require expects 1 argument");
        std::string filename = args[0]->toString();
        
        if (filename == "fs" || filename == "path") {
            return std::shared_ptr<JSValue>(std::make_shared<JSObject>());
        }
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw RuntimeError("Error: Cannot find module '" + filename + "'");
        }
        std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        Lexer lexer(code);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto ast = parser.parse();
        
        auto moduleEnv = std::make_shared<Environment>(this->environment);
        auto moduleObj = std::make_shared<JSObject>();
        auto exportsObj = std::make_shared<JSObject>();
        moduleObj->properties["exports"].value = exportsObj;
        moduleEnv->define("module", moduleObj);
        moduleEnv->define("exports", exportsObj);
        
        auto previousEnv = this->environment;
        this->environment = moduleEnv;
        try {
            for (const auto& stmt : ast->body) hoist(stmt);
            for (const auto& stmt : ast->body) execute(stmt);
        } catch (...) {
            this->environment = previousEnv;
            throw;
        }
        this->environment = previousEnv;
        
        return moduleObj->properties["exports"].value;
    }));

    // Add fs
    auto fsObj = std::make_shared<JSObject>();
    fsObj->properties["readFileSync"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("readFileSync", [](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty()) throw RuntimeError("TypeError: readFileSync expects at least 1 argument");
        std::string filename = args[0]->toString();
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw RuntimeError("Error: ENOENT: no such file or directory, open '" + filename + "'");
        }
        std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return std::shared_ptr<JSValue>(std::make_shared<JSString>(fileContent));
    }), nullptr, nullptr, true, true, true};
    environment->define("fs", fsObj);

    auto jsonObj = std::make_shared<JSObject>();
    jsonObj->prototype = objectPrototype;
    jsonObj->properties["stringify"].value = std::make_shared<JSNativeFunction>("stringify", [this](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
        
        
        std::unordered_set<void*> visited;
        std::function<std::string(std::shared_ptr<JSValue>)> stringify = [&](std::shared_ptr<JSValue> val) -> std::string {
            if (val && (val->getType() == JSValueType::OBJECT || val->getType() == JSValueType::ARRAY)) {
                if (visited.count(val.get())) {
                    throw JSException(std::make_shared<JSString>("TypeError: Converting circular structure to JSON"));
                }
                visited.insert(val.get());
            }
            
            struct VisitedGuard {
                std::unordered_set<void*>& v;
                void* ptr;
                bool isObj;
                VisitedGuard(std::unordered_set<void*>& v, void* ptr, bool isObj) : v(v), ptr(ptr), isObj(isObj) {}
                ~VisitedGuard() { if (isObj) v.erase(ptr); }
            } guard(visited, val.get(), val && (val->getType() == JSValueType::OBJECT || val->getType() == JSValueType::ARRAY));

            if (!val) return "null";
            if (val->getType() == JSValueType::NULL_TYPE) return "null";
            if (val->getType() == JSValueType::UNDEFINED) return "undefined";
            if (val->getType() == JSValueType::BOOLEAN) return val->isTruthy() ? "true" : "false";
            if (val->getType() == JSValueType::NUMBER) return val->toString();
            if (val->getType() == JSValueType::STRING) return "\"" + val->toString() + "\""; // Need escaping in full impl
            if (val->getType() == JSValueType::ARRAY) {
                auto arr = std::dynamic_pointer_cast<JSArray>(val);
                std::string res = "[";
                for (size_t i = 0; i < arr->elements.size(); ++i) {
                    if (i > 0) res += ",";
                    res += stringify(arr->elements[i]);
                }
                res += "]";
                return res;
            }
            if (val->getType() == JSValueType::OBJECT) {
                auto obj = std::dynamic_pointer_cast<JSObject>(val);
                std::string res = "{";
                bool first = true;
                for (const auto& kv : obj->properties) {
                    if (kv.second.value) { // Ignore getters for simple stringify
                        if (!first) res += ",";
                        res += "\"" + kv.first + "\":" + stringify(kv.second.value);
                        first = false;
                    }
                }
                res += "}";
                return res;
            }
            return "undefined";
        };
        
        return std::shared_ptr<JSValue>(std::make_shared<JSString>(stringify(args[0])));
    });

    jsonObj->properties["parse"].value = std::make_shared<JSNativeFunction>("parse", [this](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
        if (args.empty()) throw RuntimeError("SyntaxError: Unexpected end of JSON input");
        std::string jsonStr = "(" + args[0]->toString() + ")";
        Lexer lexer(jsonStr);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto ast = parser.parse();
        if (!ast->body.empty()) {
            if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(ast->body[0])) {
                return this->evaluate(exprStmt->expression);
            }
        }
        return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
    });

    environment->define("JSON", jsonObj);

    // Inject Polyfills for Map, Set, Array.prototype.indexOf, etc.
    std::string polyfillCode = R"(

        class EventEmitter {
            constructor() { this.events = {}; }
            on(event, cb) { 
                if(!this.events[event]) this.events[event] = [];
                this.events[event].push(cb);
            }
            emit() {
                let event = arguments[0];
                let arg1 = arguments[1];
                let arg2 = arguments[2];
                let arg3 = arguments[3];
                if(this.events[event]) {
                    let evts = this.events[event];
                    for(let i=0; i<evts.length; i++) {
                        let cb = evts[i];
                        queueMicrotask(function() { cb(arg1, arg2, arg3); });
                    }
                }
            }
        }
        
        
        let String = function(val) {
            if (val === undefined) return "";
            return val + "";
        };
        
        String.prototype.includes = function(search) { return this.indexOf(search) !== -1; };

        String.prototype.startsWith = function(search) { return this.indexOf(search) === 0; };
        String.prototype.endsWith = function(search) { 
            let idx = this.indexOf(search);
            return idx !== -1 && idx === this.length - search.length;
        };
        String.prototype.concat = function() {
            let str = String(this);
            for(let i=0; i<arguments.length; i++) str += arguments[i];
            return str;
        };
        
        function repeatString(str, count) {
            let res = "";
            for(let i=0; i<count; i++) res += str;
            return res;
        }

        String.prototype.padStart = function(targetLength, padString) {
            targetLength = targetLength >> 0;
            padString = String(padString !== undefined ? padString : ' ');
            let strThis = String(this);
            if (strThis.length > targetLength) return strThis;
            else {
                targetLength = targetLength - strThis.length;
                if (targetLength > padString.length) {
                    padString += repeatString(padString, targetLength / padString.length);
                }
                return padString.substring(0, targetLength) + strThis;
            }
        };
        String.prototype.padEnd = function(targetLength, padString) {
            targetLength = targetLength >> 0;
            padString = String(padString !== undefined ? padString : ' ');
            let strThis = String(this);
            if (strThis.length > targetLength) return strThis;
            else {
                targetLength = targetLength - strThis.length;
                if (targetLength > padString.length) {
                    padString += repeatString(padString, targetLength / padString.length);
                }
                return strThis + padString.substring(0, targetLength);
            }
        };

        class Map {
            constructor() { this._keys = []; this._values = []; }
            set(k, v) { 
                let idx = this._keys.indexOf(k);
                if (idx !== -1) this._values[idx] = v;
                else { this._keys.push(k); this._values.push(v); }
                return this;
            }
            get(k) {
                let idx = this._keys.indexOf(k);
                return idx !== -1 ? this._values[idx] : undefined;
            }
            has(k) { return this._keys.indexOf(k) !== -1; }
            clear() { this._keys = []; this._values = []; }
        }
        Map.prototype["delete"] = function(k) {
            let idx = this._keys.indexOf(k);
            if (idx !== -1) { this._keys.splice(idx, 1); this._values.splice(idx, 1); return true; }
            return false;
        };
        Object.defineProperty(Map.prototype, "size", { get: function() { return this._keys.length; } });
        
        class Set {
            constructor(iterable) { 
                this._values = []; 
                if (iterable) {
                    for (let x of iterable) this.add(x);
                }
            }
            add(v) { if (!this.has(v)) this._values.push(v); return this; }
            has(v) { return this._values.indexOf(v) !== -1; }
            clear() { this._values = []; }
        }
        Set.prototype["delete"] = function(v) {
            let idx = this._values.indexOf(v);
            if (idx !== -1) { this._values.splice(idx, 1); return true; }
            return false;
        };
        Object.defineProperty(Set.prototype, "size", { get: function() { return this._values.length; } });
        
        class Promise {
            constructor(executor) {
                this.state = "pending";
                this.value = undefined;
                this.onFulfilledCallbacks = [];
                this.onRejectedCallbacks = [];
                let self = this;
                let resolve = function(value) {
                    if (self.state === "pending") {
                        self.state = "fulfilled";
                        self.value = value;
                        for (let i = 0; i < self.onFulfilledCallbacks.length; i++) {
                            self.onFulfilledCallbacks[i](self.value);
                        }
                    }
                };
                let reject = function(reason) {
                    if (self.state === "pending") {
                        self.state = "rejected";
                        self.value = reason;
                        for (let i = 0; i < self.onRejectedCallbacks.length; i++) {
                            self.onRejectedCallbacks[i](self.value);
                        }
                    }
                };
                try { executor(resolve, reject); } catch (e) { reject(e); }
            }
            then(onFulfilled, onRejected) {
                let defaultOnFulfilled = function(v) { return v; };
                let defaultOnRejected = function(e) { throw e; };
                let actualOnFulfilled = typeof onFulfilled === "function" ? onFulfilled : defaultOnFulfilled;
                let actualOnRejected = typeof onRejected === "function" ? onRejected : defaultOnRejected;
                let self = this;
                return new Promise(function(resolve, reject) {
                    if (self.state === "fulfilled") {
                        queueMicrotask(function() {
                            try { resolve(actualOnFulfilled(self.value)); } catch (e) { reject(e); }
                        });
                    } else if (self.state === "rejected") {
                        queueMicrotask(function() {
                            try { resolve(actualOnRejected(self.value)); } catch (e) { reject(e); }
                        });
                    } else {
                        self.onFulfilledCallbacks.push(function(value) {
                            queueMicrotask(function() {
                                try { resolve(actualOnFulfilled(value)); } catch (e) { reject(e); }
                            });
                        });
                        self.onRejectedCallbacks.push(function(reason) {
                            queueMicrotask(function() {
                                try { resolve(actualOnRejected(reason)); } catch (e) { reject(e); }
                            });
                        });
                    }
                });
            }
        }
                Promise.prototype["catch"] = function(onRejected) {
            return this.then(null, onRejected);
        };
        Promise.prototype["finally"] = function(onFinally) {
            return this.then(
                function(v) { onFinally(); return v; },
                function(e) { onFinally(); throw e; }
            );
        };
    )";
    try {
        Lexer polyfillLexer(polyfillCode);
        Parser polyfillParser(polyfillLexer.tokenize());
        auto polyfillAst = polyfillParser.parse();
        for (const auto& stmt : polyfillAst->body) {
            hoist(stmt);
        }
        for (const auto& stmt : polyfillAst->body) {
            execute(stmt);
        }
    } catch (const std::exception& e) {
        std::cerr << "Polyfill Init Error: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "Polyfill Init Error: Unknown\n";
    }



    auto consoleObj = std::make_shared<JSObject>();
    consoleObj->prototype = objectPrototype;
    consoleObj->properties["log"].value = std::make_shared<JSNativeFunction>("log", builtin_console_log);
    environment->define("console", consoleObj);

    auto mathObj = std::make_shared<JSObject>();
    mathObj->prototype = objectPrototype;
    mathObj->properties["floor"].value = std::make_shared<JSNativeFunction>("floor", builtin_math_floor);
    mathObj->properties["random"].value = std::make_shared<JSNativeFunction>("random", [](const std::vector<std::shared_ptr<JSValue>>& args) {
        return std::make_shared<JSNumber>((double)rand() / RAND_MAX);
    });
    mathObj->properties["max"].value = std::make_shared<JSNativeFunction>("max", [](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(-INFINITY));
        double maxVal = args[0]->toNumber();
        for (const auto& a : args) maxVal = std::max(maxVal, a->toNumber());
        return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(maxVal));
    });
    mathObj->properties["min"].value = std::make_shared<JSNativeFunction>("min", [](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(INFINITY));
        double minVal = args[0]->toNumber();
        for (const auto& a : args) minVal = std::min(minVal, a->toNumber());
        return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(minVal));
    });
    mathObj->properties["pow"].value = std::make_shared<JSNativeFunction>("pow", [](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.size() < 2) return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(NAN));
        return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(std::pow(args[0]->toNumber(), args[1]->toNumber())));
    });
    mathObj->properties["round"].value = std::make_shared<JSNativeFunction>("round", [](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(NAN));
        return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(std::round(args[0]->toNumber())));
    });
    environment->define("Math", mathObj);
    environment->define("Array", std::make_shared<JSNativeFunction>("Array", [](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
        auto arr = std::make_shared<JSArray>();
        if (args.size() == 1 && args[0]->getType() == JSValueType::NUMBER) {
            int len = args[0]->toNumber();
            arr->elements.resize(len, std::make_shared<JSUndefined>());
        } else {
            for(auto a : args) arr->elements.push_back(a);
        }
        return arr;
    }));

    auto dateObj = std::make_shared<JSObject>();
    dateObj->properties["now"].value = std::make_shared<JSNativeFunction>("now", [](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        return std::make_shared<JSNumber>(millis);
    });
    environment->define("Date", dateObj);


    auto setTimeout = std::make_shared<JSNativeFunction>("setTimeout", [this](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty() || (args[0]->getType() != JSValueType::FUNCTION && args[0]->getType() != JSValueType::NATIVE_FUNCTION)) return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
        long long delay = 0;
        if (args.size() > 1) delay = args[1]->toNumber();
        std::vector<std::shared_ptr<JSValue>> cbArgs;
        for (size_t i = 2; i < args.size(); ++i) cbArgs.push_back(args[i]);
        
        TimerTask task;
        task.triggerTimeMs = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count() + delay;
        task.callback = args[0];
        task.args = cbArgs;
        task.id = this->nextTimerId++;
        task.isInterval = false;
        task.intervalMs = 0;
        this->timerQueue.push_back(task);
        return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(task.id));
    });
    environment->define("setTimeout", setTimeout);

    auto setInterval = std::make_shared<JSNativeFunction>("setInterval", [this](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty() || (args[0]->getType() != JSValueType::FUNCTION && args[0]->getType() != JSValueType::NATIVE_FUNCTION)) return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
        long long interval = 0;
        if (args.size() > 1) interval = args[1]->toNumber();
        std::vector<std::shared_ptr<JSValue>> cbArgs;
        for (size_t i = 2; i < args.size(); ++i) cbArgs.push_back(args[i]);
        
        TimerTask task;
        task.triggerTimeMs = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count() + interval;
        task.callback = args[0];
        task.args = cbArgs;
        task.id = this->nextTimerId++;
        task.isInterval = true;
        task.intervalMs = interval;
        this->timerQueue.push_back(task);
        return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(task.id));
    });
    environment->define("setInterval", setInterval);

    auto clearTimeout = std::make_shared<JSNativeFunction>("clearTimeout", [this](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
        int id = args[0]->toNumber();
        this->cancelledTimers.insert(id);
        for (auto& t : this->timerQueue) if (t.id == id) t.cancelled = true;
        return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
    });
    environment->define("clearTimeout", clearTimeout);
    environment->define("clearInterval", clearTimeout);

    environment->define("queueMicrotask", std::make_shared<JSNativeFunction>("queueMicrotask", [this](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty() || args[0]->getType() != JSValueType::FUNCTION) throw RuntimeError("TypeError: queueMicrotask requires a function argument");
        auto func = std::dynamic_pointer_cast<JSFunction>(args[0]);
        this->enqueueMicrotask([this, func]() {
            try {
                this->executeFunction(func, nullptr, {});
            } catch (const JSException& e) {
                std::cerr << "Uncaught (in promise) " << e.value->toString() << std::endl;
            } catch (const RuntimeError& e) {
                std::cerr << "Runtime Error (in promise): " << e.what() << std::endl;
            } catch (...) {}
        });
        return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
    }));
}

void Evaluator::runMicrotasks() {
    while (!microtaskQueue.empty()) {
        auto task = microtaskQueue.front();
        microtaskQueue.pop();
        try {
            task();
        } catch (...) {
            // Already caught inside task usually, but just in case
        }
    }
}

void Evaluator::runEventLoop() {
    runMicrotasks();
    while (!timerQueue.empty() || !microtaskQueue.empty()) {
        eventLoopTick++;
        if (eventLoopTick % 1000 == 0) { markAndSweep(); }

        runMicrotasks();
        if (timerQueue.empty()) break;
        
        auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
        bool executedAny = false;
        
        for (auto it = timerQueue.begin(); it != timerQueue.end(); ) {
            if (it->cancelled || cancelledTimers.find(it->id) != cancelledTimers.end()) {
                it = timerQueue.erase(it);
                continue;
            }
            if (now >= it->triggerTimeMs) {
                TimerTask task = *it;
                it = timerQueue.erase(it);
                
                if (task.callback->getType() == JSValueType::FUNCTION) {
                    auto func = std::dynamic_pointer_cast<JSFunction>(task.callback);
                    auto funcEnv = std::make_shared<Environment>(func->closure);
                    size_t paramCount = func->declaration->parameters.size();
                    for (size_t i = 0; i < paramCount; ++i) {
                        if (i < task.args.size()) {
                            funcEnv->define(func->declaration->parameters[i].first, task.args[i]);
                        } else {
                            funcEnv->define(func->declaration->parameters[i].first, std::make_shared<JSUndefined>());
                        }
                    }
                    try {
                        for (const auto& s : func->declaration->body->statements) hoist(s);
                        executeBlock(func->declaration->body->statements, funcEnv);
                    } catch (const ReturnException&) {}
                } else if (task.callback->getType() == JSValueType::NATIVE_FUNCTION) {
                    auto nativeFunc = std::dynamic_pointer_cast<JSNativeFunction>(task.callback);
                    nativeFunc->func(task.args);
                }
                
                if (task.isInterval && !task.cancelled && this->cancelledTimers.find(task.id) == this->cancelledTimers.end()) {
                    task.triggerTimeMs = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count() + task.intervalMs;
                    timerQueue.push_back(task);
                }
                
                runMicrotasks(); // Process microtasks right after each macrotask
                executedAny = true;
                break; // Restart iteration since we modified the queue
            } else {
                ++it;
            }
        }
        
        if (!executedAny && !timerQueue.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void Evaluator::interpret(std::shared_ptr<Program> program) {
    try {
        for (const auto& stmt : program->body) {
            hoist(stmt);
        }
        registerTDZ(program->body, environment);
        for (const auto& stmt : program->body) {
            execute(stmt);
        }
        runEventLoop();
        markAndSweep();
    } catch (const JSException& e) {
        
            std::string stackTrace = "Uncaught Error: " + e.value->toString();
            for (auto it = executionStack.rbegin(); it != executionStack.rend(); ++it) {
                stackTrace += "\n    at " + *it + "()";
            }
            std::cerr << stackTrace << std::endl;

    } catch (const RuntimeError& e) {
        std::cerr << "Runtime Error: " << e.what() << std::endl;
    } catch (const ReturnException& e) {
        // Uncaught return at top level (ignored)
    }
}

void Evaluator::executeBlock(const std::vector<std::shared_ptr<Statement>>& statements, std::shared_ptr<Environment> env) {
    auto previous = environment;
    try {
        environment = env;
        registerTDZ(statements, environment);
        for (const auto& stmt : statements) {
            execute(stmt);
        }
        environment = previous;
    } catch (...) {
        environment = previous;
        throw;
    }
}

void Evaluator::registerTDZ(const std::vector<std::shared_ptr<Statement>>& statements, std::shared_ptr<Environment> env) {
    for (const auto& stmt : statements) {
        if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(stmt)) {
            if (!varDecl->isVar) {
                env->defineTdz(varDecl->name, varDecl->isConst);
            }
        } else if (auto destDecl = std::dynamic_pointer_cast<DestructuringDeclaration>(stmt)) {
            if (!destDecl->isVar) {
                for (const auto& name : destDecl->names) {
                    env->defineTdz(name, destDecl->isConst);
                }
            }
        } else if (auto classDecl = std::dynamic_pointer_cast<ClassDeclaration>(stmt)) {
            env->defineTdz(classDecl->name, false);
        }
    }
}

void Evaluator::execVariableDeclaration(VariableDeclaration* varDecl) {
        std::shared_ptr<JSValue> value = std::make_shared<JSUndefined>();
        if (varDecl->initializer) {
            value = evaluate(varDecl->initializer);
        }
        environment->define(varDecl->name, value, varDecl->isConst);
}

void Evaluator::execDestructuringDeclaration(DestructuringDeclaration* destDecl) {
        auto value = evaluate(destDecl->initializer);
        if (destDecl->isArray) {
            if (value->getType() == JSValueType::ARRAY) {
                auto arr = std::dynamic_pointer_cast<JSArray>(value);
                for (size_t i = 0; i < destDecl->names.size(); ++i) {
                    if (i < arr->elements.size()) {
                        environment->define(destDecl->names[i], arr->elements[i], destDecl->isConst);
                    } else {
                        environment->define(destDecl->names[i], std::make_shared<JSUndefined>(), destDecl->isConst);
                    }
                }
            } else {
                throw RuntimeError("TypeError: rhs is not iterable");
            }
        } else {
            if (value->getType() == JSValueType::OBJECT || value->getType() == JSValueType::FUNCTION) {
                auto obj = std::dynamic_pointer_cast<JSObject>(value);
                for (size_t i = 0; i < destDecl->names.size(); ++i) {
                    std::string key = destDecl->names[i];
                    if (obj->properties.count(key)) {
                        environment->define(key, obj->properties[key].value, destDecl->isConst);
                    } else {
                        environment->define(key, std::make_shared<JSUndefined>(), destDecl->isConst);
                    }
                }
            } else {
                throw RuntimeError("TypeError: Cannot destructure property of primitive");
            }
        }
}

void Evaluator::execThrowStatement(ThrowStatement* throwStmt) {
        throw JSException(evaluate(throwStmt->argument));
}

void Evaluator::execTryStatement(TryStatement* tryStmt) {
        try {
            executeBlock(tryStmt->block->statements, std::make_shared<Environment>(environment));
        } catch (const JSException& e) {
            if (tryStmt->handler) {
                auto catchEnv = std::make_shared<Environment>(environment);
                if (!tryStmt->param.empty()) {
                    catchEnv->define(tryStmt->param, e.value);
                }
                executeBlock(tryStmt->handler->statements, catchEnv);
            }
        } catch (const RuntimeError& e) {
            if (tryStmt->handler) {
                auto catchEnv = std::make_shared<Environment>(environment);
                if (!tryStmt->param.empty()) {
                    catchEnv->define(tryStmt->param, std::make_shared<JSString>(e.what()));
                }
                executeBlock(tryStmt->handler->statements, catchEnv);
            }
        }
        if (tryStmt->finalizer) {
            executeBlock(tryStmt->finalizer->statements, std::make_shared<Environment>(environment));
        }
}

void Evaluator::execExpressionStatement(ExpressionStatement* exprStmt) {
        evaluate(exprStmt->expression);
}

void Evaluator::execIfStatement(IfStatement* ifStmt) {
        auto condition = evaluate(ifStmt->condition);
        if (condition->isTruthy()) {
            execute(ifStmt->consequent);
        } else if (ifStmt->alternate) {
            execute(ifStmt->alternate);
        }
}

void Evaluator::execBlockStatement(BlockStatement* blockStmt) {
        auto blockEnv = std::make_shared<Environment>(environment);
        executeBlock(blockStmt->statements, blockEnv);
}

void Evaluator::execWhileStatement(WhileStatement* whileStmt) {
        while (evaluate(whileStmt->condition)->isTruthy()) {
            try {
                execute(whileStmt->body);
            } catch (const BreakException&) {
                break;
            } catch (const ContinueException&) {
                continue;
            } catch (const ReturnException&) {
                throw;
            }
        }
}

void Evaluator::execDoWhileStatement(DoWhileStatement* doWhileStmt) {
        do {
            try {
                execute(doWhileStmt->body);
            } catch (const BreakException&) {
                break;
            } catch (const ContinueException&) {
                continue;
            } catch (const ReturnException&) {
                throw;
            }
        } while (evaluate(doWhileStmt->condition)->isTruthy());
}

void Evaluator::execSwitchStatement(SwitchStatement* switchStmt) {
        auto discriminant = evaluate(switchStmt->discriminant);
        bool match = false;
        try {
            for (const auto& caseClause : switchStmt->cases) {
                if (!match && caseClause.test) {
                    auto testVal = evaluate(caseClause.test);
                    if (discriminant->getType() == testVal->getType() && discriminant->toString() == testVal->toString()) {
                        match = true;
                    }
                } else if (!match && !caseClause.test) { // default
                    match = true;
                }
                if (match) {
                    for (const auto& caseStmt : caseClause.consequent) {
                        execute(caseStmt);
                    }
                }
            }
        } catch (const BreakException&) {
            // switch ended
        } catch (const ReturnException&) {
            throw;
        }
}

void Evaluator::execForStatement(ForStatement* forStmt) {
        auto forEnv = std::make_shared<Environment>(environment);
        auto previous = environment;
        environment = forEnv;

        try {
            if (forStmt->init) {
                execute(forStmt->init);
            }
            while (forStmt->condition ? evaluate(forStmt->condition)->isTruthy() : true) {
                auto iterEnv = std::make_shared<Environment>(previous);
                
                for (const auto& kv : forEnv->getLocalValues()) {
                    iterEnv->define(kv.first, kv.second.value, kv.second.isConst);
                }

                auto previousIter = environment;
                environment = iterEnv;

                try {
                    execute(forStmt->body);
                } catch (const BreakException&) {
                    environment = previousIter;
                    break;
                } catch (const ContinueException&) {
                    environment = previousIter;
                    // continue with update
                } catch (const ReturnException&) {
                    environment = previousIter;
                    throw;
                }
                
                for (const auto& kv : iterEnv->getLocalValues()) {
                    if (forEnv->getLocalValues().count(kv.first)) {
                        forEnv->assign(kv.first, kv.second.value);
                    }
                }
                
                environment = previousIter;

                if (forStmt->update) {
                    evaluate(forStmt->update);
                }
            }
        } catch (...) {
            environment = previous;
            throw;
        }
        environment = previous;
}

void Evaluator::execForInStatement(ForInStatement* forInStmt) {
        auto rightVal = evaluate(forInStmt->right);
        auto forEnv = std::make_shared<Environment>(environment);
        auto previous = environment;
        environment = forEnv;
        
        std::string varName = "";
        if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(forInStmt->left)) varName = varDecl->name;
        else if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(forInStmt->left)) {
            if (auto id = std::dynamic_pointer_cast<Identifier>(exprStmt->expression)) varName = id->name;
        }

        try {
            if (rightVal->getType() == JSValueType::OBJECT) {
                auto obj = std::dynamic_pointer_cast<JSObject>(rightVal);
                for (const auto& pair : obj->properties) {
                    environment->define(varName, std::make_shared<JSString>(pair.first));
                    try {
                        execute(forInStmt->body);
                    } catch (const BreakException&) { break; }
                    catch (const ContinueException&) { continue; }
                    catch (const ReturnException&) { throw; }
                }
            } else if (rightVal->getType() == JSValueType::ARRAY) {
                auto arr = std::dynamic_pointer_cast<JSArray>(rightVal);
                for (size_t i = 0; i < arr->elements.size(); ++i) {
                    environment->define(varName, std::make_shared<JSString>(std::to_string(i)));
                    try {
                        execute(forInStmt->body);
                    } catch (const BreakException&) { break; }
                    catch (const ContinueException&) { continue; }
                    catch (const ReturnException&) { throw; }
                }
            }
        } catch (...) {
            environment = previous;
            throw;
        }
        environment = previous;
}

void Evaluator::execForOfStatement(ForOfStatement* forOfStmt) {
        auto rightVal = evaluate(forOfStmt->right);
        auto forEnv = std::make_shared<Environment>(environment);
        auto previous = environment;
        environment = forEnv;
        
        std::string varName = "";
        if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(forOfStmt->left)) varName = varDecl->name;
        else if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(forOfStmt->left)) {
            if (auto id = std::dynamic_pointer_cast<Identifier>(exprStmt->expression)) varName = id->name;
        }

        try {
            std::shared_ptr<JSValue> iteratorFunc;
            if (rightVal->getType() == JSValueType::OBJECT || rightVal->getType() == JSValueType::ARRAY) {
                auto jsObj = std::dynamic_pointer_cast<JSObject>(rightVal);
                auto currentObj = jsObj;
                while (currentObj) {
                    if (currentObj->properties.count("__iterator__")) {
                        iteratorFunc = currentObj->properties["__iterator__"].value;
                        break;
                    }
                    currentObj = std::dynamic_pointer_cast<JSObject>(currentObj->prototype);
                }
            }

            if (iteratorFunc && (iteratorFunc->getType() == JSValueType::FUNCTION || iteratorFunc->getType() == JSValueType::NATIVE_FUNCTION)) {
                auto iteratorObj = executeFunction(iteratorFunc, rightVal, {});
                if (iteratorObj->getType() != JSValueType::OBJECT) throw RuntimeError("TypeError: Result of the Symbol.iterator method is not an object");
                auto iter = std::dynamic_pointer_cast<JSObject>(iteratorObj);
                
                while (true) {
                    std::shared_ptr<JSValue> nextFunc;
                    auto currentIterObj = iter;
                    while (currentIterObj) {
                        if (currentIterObj->properties.count("next")) {
                            nextFunc = currentIterObj->properties["next"].value;
                            break;
                        }
                        currentIterObj = std::dynamic_pointer_cast<JSObject>(currentIterObj->prototype);
                    }
                    if (!nextFunc) throw RuntimeError("TypeError: iterator.next is not a function");
                    
                    auto resultObj = executeFunction(nextFunc, iter, {});
                    if (resultObj->getType() != JSValueType::OBJECT) throw RuntimeError("TypeError: Iterator result is not an object");
                    auto result = std::dynamic_pointer_cast<JSObject>(resultObj);
                    
                    bool done = false;
                    if (result->properties.count("done")) done = result->properties["done"].value->isTruthy();
                    if (done) break;
                    
                    auto val = std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
                    if (result->properties.count("value")) val = result->properties["value"].value;
                    
                    environment->define(varName, val);
                    try {
                        execute(forOfStmt->body);
                    } catch (const BreakException&) { break; }
                    catch (const ContinueException&) { continue; }
                    catch (const ReturnException&) { throw; }
                }
            } else if (rightVal->getType() == JSValueType::ARRAY) {
                auto arr = std::dynamic_pointer_cast<JSArray>(rightVal);
                for (const auto& elem : arr->elements) {
                    environment->define(varName, elem);
                    try {
                        execute(forOfStmt->body);
                    } catch (const BreakException&) { break; }
                    catch (const ContinueException&) { continue; }
                    catch (const ReturnException&) { throw; }
                }
            } else if (rightVal->getType() == JSValueType::STRING) {
                auto str = std::dynamic_pointer_cast<JSString>(rightVal);
                for (char c : str->value) {
                    environment->define(varName, std::make_shared<JSString>(std::string(1, c)));
                    try {
                        execute(forOfStmt->body);
                    } catch (const BreakException&) { break; }
                    catch (const ContinueException&) { continue; }
                    catch (const ReturnException&) { throw; }
                }
            } else {
                throw RuntimeError("TypeError: rhs is not iterable");
            }
        } catch (...) {
            environment = previous;
            throw;
        }
        environment = previous;
}

void Evaluator::execFunctionDeclaration(FunctionDeclaration* funcDecl) {
        // Handled in hoist phase
}

void Evaluator::execClassDeclaration(ClassDeclaration* classDecl) {
        std::shared_ptr<JSFunction> classConstructor;
        
        for (auto method : classDecl->methods) {
            if (method->name == "constructor") {
                auto decl = std::make_shared<FunctionDeclaration>(classDecl->name, method->parameters, method->body);
                classConstructor = std::make_shared<JSFunction>(decl, environment);
                break;
            }
        }
        if (!classConstructor) {
            auto decl = std::make_shared<FunctionDeclaration>(classDecl->name, std::vector<std::pair<std::string, std::shared_ptr<Expression>>>(), std::make_shared<BlockStatement>(std::vector<std::shared_ptr<Statement>>{}));
            classConstructor = std::make_shared<JSFunction>(decl, environment);
        }
        
        auto prototype = std::make_shared<JSObject>();
        if (classDecl->superClass) {
            auto superVal = evaluate(classDecl->superClass);
            if (superVal->getType() != JSValueType::FUNCTION) throw RuntimeError("TypeError: Superclass must be a function");
            auto superFunc = std::dynamic_pointer_cast<JSFunction>(superVal);
            if (superFunc->prototypeProperty) {
                prototype->prototype = std::dynamic_pointer_cast<JSObject>(superFunc->prototypeProperty);
            }
        } else {
            prototype->prototype = objectPrototype;
        }
        
        for (auto method : classDecl->methods) {
            if (method->name == "constructor") continue;
            auto decl = std::make_shared<FunctionDeclaration>(method->name, method->parameters, method->body);
            auto methodFunc = std::make_shared<JSFunction>(decl, environment);
            prototype->properties[method->name].value = methodFunc;
        }
        
        classConstructor->prototypeProperty = prototype;
        prototype->properties["constructor"].value = classConstructor;
        environment->define(classDecl->name, classConstructor);
}

void Evaluator::execReturnStatement(ReturnStatement* returnStmt) {
        std::shared_ptr<JSValue> value = std::make_shared<JSUndefined>();
        if (returnStmt->argument) {
            value = evaluate(returnStmt->argument);
        }
        throw ReturnException(value);
}

void Evaluator::execute(std::shared_ptr<Statement> stmt) {
    if (!stmt) return;
    stmt->execute(this);
}



std::shared_ptr<JSValue> Evaluator::executeFunction(std::shared_ptr<JSValue> callee, std::shared_ptr<JSValue> thisContext, const std::vector<std::shared_ptr<JSValue>>& args) {
    
    std::string funcName = "anonymous";
    if (callee->getType() == JSValueType::NATIVE_FUNCTION) {
        funcName = std::dynamic_pointer_cast<JSNativeFunction>(callee)->name;
    } else if (callee->getType() == JSValueType::FUNCTION) {
        auto fn = std::dynamic_pointer_cast<JSFunction>(callee);
        if (!fn->declaration->name.empty()) funcName = fn->declaration->name;
    }
    
    class ExecutionStackGuard {
        std::vector<std::string>& stack;
    public:
        ExecutionStackGuard(std::vector<std::string>& s, const std::string& name) : stack(s) {
            stack.push_back(name);
        }
        ~ExecutionStackGuard() {
            if (!stack.empty()) stack.pop_back();
        }
    };
    ExecutionStackGuard execGuard(executionStack, funcName);
if (callee->getType() == JSValueType::NATIVE_FUNCTION) {
        auto nativeFunc = std::dynamic_pointer_cast<JSNativeFunction>(callee);
        auto previousThis = lastThisContext;
        lastThisContext = thisContext;
        auto ret = nativeFunc->func(args);
        lastThisContext = previousThis;
        return ret;
    }
    if (callee->getType() == JSValueType::FUNCTION) {
        auto func = std::dynamic_pointer_cast<JSFunction>(callee);
        auto funcEnv = std::make_shared<Environment>(func->closure);
        
        if (!func->isArrow) {
            funcEnv->define("this", thisContext);
            auto argsObj = std::make_shared<JSObject>();
            argsObj->prototype = objectPrototype;
            argsObj->properties["length"].value = std::make_shared<JSNumber>(args.size());
            for (size_t i = 0; i < args.size(); ++i) {
                argsObj->properties[std::to_string(i)].value = args[i];
            }
            funcEnv->define("arguments", argsObj);
        } else {
            // Arrow functions inherit 'this' from their closure environment automatically
        }

        size_t paramCount = func->declaration->parameters.size();
        for (size_t i = 0; i < paramCount; ++i) {
            auto paramName = func->declaration->parameters[i].first;
            auto defaultExpr = func->declaration->parameters[i].second;

            if (func->declaration->hasRest && i == paramCount - 1) {
                auto restArr = std::make_shared<JSArray>();
                for (size_t j = i; j < args.size(); ++j) restArr->elements.push_back(args[j]);
                funcEnv->define(paramName, restArr);
            } else if (i < args.size() && args[i]->getType() != JSValueType::UNDEFINED) {
                funcEnv->define(paramName, args[i]);
            } else if (defaultExpr) {
                auto previousEnv = environment;
                environment = funcEnv;
                auto defVal = evaluate(defaultExpr);
                environment = previousEnv;
                funcEnv->define(paramName, defVal);
            } else {
                funcEnv->define(paramName, std::make_shared<JSUndefined>());
            }
        }
        try {
            for (const auto& s : func->declaration->body->statements) hoist(s);
            executeBlock(func->declaration->body->statements, funcEnv);
        } catch (const ReturnException& ret) {
            return ret.value;
        }
        return std::make_shared<JSUndefined>();
    }
    throw RuntimeError("TypeError: callee is not a function");
}

std::shared_ptr<JSValue> Evaluator::evalNumberLiteral(NumberLiteral* num) {
        return std::make_shared<JSNumber>(num->value);
}

std::shared_ptr<JSValue> Evaluator::evalStringLiteral(StringLiteral* str) {
        return std::make_shared<JSString>(str->value);
}

std::shared_ptr<JSValue> Evaluator::evalBooleanLiteral(BooleanLiteral* boolLit) {
        return std::make_shared<JSBoolean>(boolLit->value);
}

std::shared_ptr<JSValue> Evaluator::evalIdentifier(Identifier* id) {
        if (id->name == "undefined") return std::make_shared<JSUndefined>();
        if (id->name == "null") return std::make_shared<JSNull>();
        return environment->get(id->name);
}

std::shared_ptr<JSValue> Evaluator::evalArrayLiteral(ArrayLiteral* arrLit) {
        auto arr = std::make_shared<JSArray>();
        for (const auto& elem : arrLit->elements) {
            if (auto spread = std::dynamic_pointer_cast<SpreadElement>(elem)) {
                auto spreadVal = evaluate(spread->argument);
                if (spreadVal->getType() == JSValueType::ARRAY) {
                    auto spreadArr = std::dynamic_pointer_cast<JSArray>(spreadVal);
                    arr->elements.insert(arr->elements.end(), spreadArr->elements.begin(), spreadArr->elements.end());
                } else if (spreadVal->getType() == JSValueType::STRING) {
                    auto strVal = std::dynamic_pointer_cast<JSString>(spreadVal);
                    for (char c : strVal->value) {
                        arr->elements.push_back(std::make_shared<JSString>(std::string(1, c)));
                    }
                } else {
                    throw RuntimeError("Spread argument is not iterable");
                }
            } else {
                arr->elements.push_back(evaluate(elem));
            }
        }
        return arr;
}

std::shared_ptr<JSValue> Evaluator::evalObjectLiteral(ObjectLiteral* objLit) {
        auto obj = std::make_shared<JSObject>();
        obj->prototype = objectPrototype;
        for (const auto& prop : objLit->properties) {
            obj->properties[prop.key].value = evaluate(prop.value);
        }
        return obj;
}

std::shared_ptr<JSValue> Evaluator::evalFunctionExpression(FunctionExpression* funcExpr) {
        auto decl = std::make_shared<FunctionDeclaration>(funcExpr->name, funcExpr->parameters, funcExpr->body);
        return std::make_shared<JSFunction>(decl, environment);
}

std::shared_ptr<JSValue> Evaluator::evalArrowFunctionExpression(ArrowFunctionExpression* arrowExpr) {
        auto decl = std::make_shared<FunctionDeclaration>("", arrowExpr->parameters, arrowExpr->body);
        auto func = std::make_shared<JSFunction>(decl, environment);
        func->isArrow = true;
        return func;
}

std::shared_ptr<JSValue> Evaluator::evalTemplateLiteralExpression(TemplateLiteralExpression* tpl) {
        std::string result = "";
        for (const auto& part : tpl->parts) {
            auto val = evaluate(part);
            result += val->toString();
        }
        return std::make_shared<JSString>(result);
}

std::shared_ptr<JSValue> Evaluator::evalRegexLiteralExpression(RegexLiteralExpression* regex) {
        auto obj = std::make_shared<JSObject>();
        obj->properties["source"].value = std::make_shared<JSString>(regex->pattern);
        obj->properties["flags"].value = std::make_shared<JSString>(regex->flags);
        
        obj->properties["test"].value = std::make_shared<JSNativeFunction>("test", [regex](const std::vector<std::shared_ptr<JSValue>>& args) {
            if (args.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSBoolean>(false));
            std::string str = args[0]->toString();
            try {
                std::regex_constants::syntax_option_type opt = std::regex_constants::ECMAScript;
                if (regex->flags.find('i') != std::string::npos) opt |= std::regex_constants::icase;
                
                std::string pat = regex->pattern;
                if (pat.length() >= 2 && pat.front() == '/' && pat.back() == '/') {
                    pat = pat.substr(1, pat.length() - 2);
                }
                
                std::regex r(pat, opt);
                bool match = std::regex_search(str, r);
                return std::shared_ptr<JSValue>(std::make_shared<JSBoolean>(match));
            } catch (...) {
                return std::shared_ptr<JSValue>(std::make_shared<JSBoolean>(false));
            }
        });

        return obj;
}

std::shared_ptr<JSValue> Evaluator::evalConditionalExpression(ConditionalExpression* condExpr) {
        auto testVal = evaluate(condExpr->test);
        return testVal->isTruthy() ? evaluate(condExpr->consequent) : evaluate(condExpr->alternate);
}

std::shared_ptr<JSValue> Evaluator::evalAssignmentExpression(AssignmentExpression* assign) {
        auto value = evaluate(assign->value);
        std::string varName;
        
        if (auto id = std::dynamic_pointer_cast<Identifier>(assign->left)) {
            varName = id->name;
            if (assign->op != TokenType::ASSIGN) {
                auto current = environment->get(varName);
                if (assign->op == TokenType::PLUS_ASSIGN) {
                    if (current->getType() == JSValueType::STRING || value->getType() == JSValueType::STRING) {
                        value = std::make_shared<JSString>(current->toString() + value->toString());
                    } else {
                        value = std::make_shared<JSNumber>(current->toNumber() + value->toNumber());
                    }
                } else if (assign->op == TokenType::MINUS_ASSIGN) {
                    value = std::make_shared<JSNumber>(current->toNumber() - value->toNumber());
                } else if (assign->op == TokenType::MULTIPLY_ASSIGN) {
                    value = std::make_shared<JSNumber>(current->toNumber() * value->toNumber());
                } else if (assign->op == TokenType::DIVIDE_ASSIGN) {
                    value = std::make_shared<JSNumber>(current->toNumber() / value->toNumber());
                }
            }
            try {
                environment->assign(varName, value);
            } catch (const RuntimeError& r) {
                throw JSException(std::make_shared<JSString>(r.what()));
            }
            return value;
        } else if (auto mem = std::dynamic_pointer_cast<MemberExpression>(assign->left)) {
            auto obj = evaluate(mem->object);
            if (obj->getType() != JSValueType::OBJECT && obj->getType() != JSValueType::ARRAY && obj->getType() != JSValueType::FUNCTION) {
                throw RuntimeError("TypeError: Cannot assign to property of primitive or null/undefined");
            }
            
            std::string propName;
            if (!mem->computed) {
                if (auto strProp = std::dynamic_pointer_cast<StringLiteral>(mem->property)) {
                    propName = strProp->value;
                } else if (auto idProp = std::dynamic_pointer_cast<Identifier>(mem->property)) {
                    propName = idProp->name;
                }
            } else {
                propName = evaluate(mem->property)->toString();
            }
            
            std::shared_ptr<JSValue> current = std::make_shared<JSUndefined>();
            if (obj->getType() == JSValueType::OBJECT || obj->getType() == JSValueType::FUNCTION) {
                if (obj->getType() == JSValueType::FUNCTION && propName == "prototype") {
                    auto funcObj = std::dynamic_pointer_cast<JSFunction>(obj);
                    if (funcObj && funcObj->prototypeProperty) current = funcObj->prototypeProperty;
                } else {
                    auto jsObj = std::dynamic_pointer_cast<JSObject>(obj);
                    if (jsObj && jsObj->properties.count(propName)) current = jsObj->properties[propName].value;
                }
            } else if (obj->getType() == JSValueType::ARRAY) {
                auto arr = std::dynamic_pointer_cast<JSArray>(obj);
                if (propName == "length") {
                    current = std::make_shared<JSNumber>(arr->elements.size());
                } else {
                    try {
                        int idx = std::stoi(propName);
                        if (idx >= 0 && idx < arr->elements.size()) current = arr->elements[idx];
                    } catch (...) {}
                }
            } else if (obj->getType() == JSValueType::STRING) {
                if (propName == "length") {
                    current = std::make_shared<JSNumber>(std::dynamic_pointer_cast<JSString>(obj)->value.length());
                }
            }
            
            if (assign->op != TokenType::ASSIGN) {
                if (assign->op == TokenType::PLUS_ASSIGN) {
                    if (current->getType() == JSValueType::STRING || value->getType() == JSValueType::STRING) value = std::make_shared<JSString>(current->toString() + value->toString());
                    else value = std::make_shared<JSNumber>(current->toNumber() + value->toNumber());
                } else if (assign->op == TokenType::MINUS_ASSIGN) value = std::make_shared<JSNumber>(current->toNumber() - value->toNumber());
                else if (assign->op == TokenType::MULTIPLY_ASSIGN) value = std::make_shared<JSNumber>(current->toNumber() * value->toNumber());
                else if (assign->op == TokenType::DIVIDE_ASSIGN) value = std::make_shared<JSNumber>(current->toNumber() / value->toNumber());
            }
            
            if (obj->getType() == JSValueType::OBJECT || obj->getType() == JSValueType::FUNCTION) {
                if (obj->getType() == JSValueType::FUNCTION && propName == "prototype") {
                    auto funcObj = std::dynamic_pointer_cast<JSFunction>(obj);
                    if (funcObj) {
                        funcObj->prototypeProperty = value;
                        return value;
                    }
                }
                auto jsObj = std::dynamic_pointer_cast<JSObject>(obj);
                if (!jsObj) throw RuntimeError("TypeError: Cannot set property on non-object");
                // Search for setter in prototype chain
                auto currentObj = jsObj;
                bool setterFound = false;
                while (currentObj) {
                    if (currentObj->properties.count(propName)) {
                        auto& desc = currentObj->properties[propName];
                        if (desc.set) {
                            executeFunction(desc.set, jsObj, {value});
                            setterFound = true;
                            break;
                        }
                        if (!desc.writable) throw RuntimeError("TypeError: Cannot assign to read only property '" + propName + "'");
                        break; // Stop at first property found if no setter
                    }
                    currentObj = std::dynamic_pointer_cast<JSObject>(currentObj->prototype);
                }
                if (!setterFound) {
                    jsObj->properties[propName].value = value;
                }
            } else if (obj->getType() == JSValueType::ARRAY) {
                auto arr = std::dynamic_pointer_cast<JSArray>(obj);
                try {
                    int idx = std::stoi(propName);
                    if (idx >= 0) {
                        while (idx >= arr->elements.size()) arr->elements.push_back(std::make_shared<JSUndefined>());
                        arr->elements[idx] = value;
                    }
                } catch (...) {}
            }
            return value;
        }
        throw RuntimeError("ReferenceError: Invalid left-hand side in assignment");
}

std::shared_ptr<JSValue> Evaluator::evalUpdateExpression(UpdateExpression* update) {
        std::string varName;
        std::shared_ptr<JSValue> current = std::make_shared<JSUndefined>();
        std::shared_ptr<JSObject> targetObj;
        std::string propName;
        std::shared_ptr<JSArray> targetArr;
        int arrIdx = -1;
        
        if (auto id = std::dynamic_pointer_cast<Identifier>(update->argument)) {
            varName = id->name;
            current = environment->get(varName);
        } else if (auto mem = std::dynamic_pointer_cast<MemberExpression>(update->argument)) {
            auto obj = evaluate(mem->object);
            if (!mem->computed) {
                if (auto strProp = std::dynamic_pointer_cast<StringLiteral>(mem->property)) propName = strProp->value;
                else if (auto idProp = std::dynamic_pointer_cast<Identifier>(mem->property)) propName = idProp->name;
            } else {
                propName = evaluate(mem->property)->toString();
            }
            if (obj->getType() == JSValueType::OBJECT || obj->getType() == JSValueType::FUNCTION) {
                targetObj = std::dynamic_pointer_cast<JSObject>(obj);
                auto currentObj = targetObj;
                while (currentObj) {
                    if (currentObj->properties.count(propName)) {
                        auto& desc = currentObj->properties[propName];
                        if (desc.get) {
                            current = executeFunction(desc.get, targetObj, {});
                            break;
                        }
                        current = desc.value ? desc.value : std::make_shared<JSUndefined>();
                        break;
                    }
                    currentObj = std::dynamic_pointer_cast<JSObject>(currentObj->prototype);
                }
            } else if (obj->getType() == JSValueType::ARRAY) {
                targetArr = std::dynamic_pointer_cast<JSArray>(obj);
                try {
                    arrIdx = std::stoi(propName);
                    if (arrIdx >= 0 && arrIdx < targetArr->elements.size()) current = targetArr->elements[arrIdx];
                } catch (...) { }
            } else {
                throw RuntimeError("TypeError: Cannot modify property of primitive");
            }
        } else {
            throw RuntimeError("ReferenceError: Invalid left-hand side in update");
        }
        
        double oldVal = current->toNumber();
        double newVal = oldVal + (update->op == TokenType::INCREMENT ? 1 : -1);
        auto newJsVal = std::make_shared<JSNumber>(newVal);
        
        if (!varName.empty()) {
            environment->assign(varName, newJsVal);
        } else if (targetObj) {
            auto currentObj = targetObj;
            bool setterFound = false;
            while (currentObj) {
                if (currentObj->properties.count(propName)) {
                    auto& desc = currentObj->properties[propName];
                    if (desc.set) {
                        executeFunction(desc.set, targetObj, {newJsVal});
                        setterFound = true;
                        break;
                    }
                    if (!desc.writable) throw RuntimeError("TypeError: Cannot assign to read only property '" + propName + "'");
                    break;
                }
                currentObj = std::dynamic_pointer_cast<JSObject>(currentObj->prototype);
            }
            if (!setterFound) targetObj->properties[propName].value = newJsVal;
        } else if (targetArr && arrIdx >= 0) {
            while (arrIdx >= targetArr->elements.size()) targetArr->elements.push_back(std::make_shared<JSUndefined>());
            targetArr->elements[arrIdx] = newJsVal;
        }
        
        return update->isPrefix ? newJsVal : std::make_shared<JSNumber>(oldVal);
}

std::shared_ptr<JSValue> Evaluator::evalUnaryExpression(UnaryExpression* unary) {
        auto arg = evaluate(unary->argument);
        if (unary->op == TokenType::MINUS) {
            return std::make_shared<JSNumber>(-arg->toNumber());
        } else if (unary->op == TokenType::LOGICAL_NOT) {
            return std::make_shared<JSBoolean>(!arg->isTruthy());
        } else if (unary->op == TokenType::BITWISE_NOT) {
            return std::make_shared<JSNumber>(~(static_cast<int>(arg->toNumber())));
        } else if (unary->op == TokenType::TYPEOF) {
            switch (arg->getType()) {
                case JSValueType::NUMBER: return std::make_shared<JSString>("number");
                case JSValueType::STRING: return std::make_shared<JSString>("string");
                case JSValueType::BOOLEAN: return std::make_shared<JSString>("boolean");
                case JSValueType::UNDEFINED: return std::make_shared<JSString>("undefined");
                case JSValueType::NULL_TYPE: return std::make_shared<JSString>("object");
                case JSValueType::OBJECT: 
                case JSValueType::ARRAY: return std::make_shared<JSString>("object");
                case JSValueType::FUNCTION:
                case JSValueType::NATIVE_FUNCTION: return std::make_shared<JSString>("function");
            }
        }
    return std::make_shared<JSUndefined>();
}

std::shared_ptr<JSValue> Evaluator::evalBinaryExpression(BinaryExpression* binary) {
        auto left = evaluate(binary->left);
        if (binary->op == TokenType::LOGICAL_AND) {
            return left->isTruthy() ? evaluate(binary->right) : left;
        }
        if (binary->op == TokenType::LOGICAL_OR) {
            return left->isTruthy() ? left : evaluate(binary->right);
        }

        auto right = evaluate(binary->right);
        switch (binary->op) {
            case TokenType::PLUS:
                if (left->getType() == JSValueType::STRING || right->getType() == JSValueType::STRING ||
                    left->getType() == JSValueType::ARRAY || right->getType() == JSValueType::ARRAY) {
                    return std::make_shared<JSString>(left->toString() + right->toString());
                }
                return std::make_shared<JSNumber>(left->toNumber() + right->toNumber());
            case TokenType::MINUS: return std::make_shared<JSNumber>(left->toNumber() - right->toNumber());
            case TokenType::MULTIPLY: return std::make_shared<JSNumber>(left->toNumber() * right->toNumber());
            case TokenType::DIVIDE: return std::make_shared<JSNumber>(left->toNumber() / right->toNumber());
            case TokenType::MODULO: return std::make_shared<JSNumber>(std::fmod(left->toNumber(), right->toNumber()));
            case TokenType::POWER: return std::make_shared<JSNumber>(std::pow(left->toNumber(), right->toNumber()));
            case TokenType::BITWISE_AND: return std::make_shared<JSNumber>(static_cast<int32_t>(left->toNumber()) & static_cast<int32_t>(right->toNumber()));
            case TokenType::BITWISE_OR: return std::make_shared<JSNumber>(static_cast<int32_t>(left->toNumber()) | static_cast<int32_t>(right->toNumber()));
            case TokenType::BITWISE_XOR: return std::make_shared<JSNumber>(static_cast<int32_t>(left->toNumber()) ^ static_cast<int32_t>(right->toNumber()));
            case TokenType::LEFT_SHIFT: return std::make_shared<JSNumber>(static_cast<int32_t>(left->toNumber()) << (static_cast<uint32_t>(right->toNumber()) & 0x1F));
            case TokenType::RIGHT_SHIFT: return std::make_shared<JSNumber>(static_cast<int32_t>(left->toNumber()) >> (static_cast<uint32_t>(right->toNumber()) & 0x1F));
            case TokenType::UNSIGNED_RIGHT_SHIFT: return std::make_shared<JSNumber>(static_cast<uint32_t>(left->toNumber()) >> (static_cast<uint32_t>(right->toNumber()) & 0x1F));
            case TokenType::EQUAL:
                if (left->getType() == JSValueType::NULL_TYPE && right->getType() == JSValueType::UNDEFINED) return std::make_shared<JSBoolean>(true);
                if (left->getType() == JSValueType::UNDEFINED && right->getType() == JSValueType::NULL_TYPE) return std::make_shared<JSBoolean>(true);
                if (left->getType() != right->getType()) {
                    if ((left->getType() == JSValueType::NUMBER && right->getType() == JSValueType::STRING) ||
                        (left->getType() == JSValueType::STRING && right->getType() == JSValueType::NUMBER) ||
                        (left->getType() == JSValueType::BOOLEAN) || (right->getType() == JSValueType::BOOLEAN)) {
                        return std::make_shared<JSBoolean>(left->toNumber() == right->toNumber());
                    }
                }
                // Fallthrough to strict equal if types match or aren't easily coercible
            case TokenType::STRICT_EQUAL:
                if (left->getType() == JSValueType::NUMBER && right->getType() == JSValueType::NUMBER)
                    return std::make_shared<JSBoolean>(left->toNumber() == right->toNumber());
                if (left->getType() == JSValueType::STRING && right->getType() == JSValueType::STRING)
                    return std::make_shared<JSBoolean>(left->toString() == right->toString());
                if ((left->getType() == JSValueType::OBJECT && right->getType() == JSValueType::OBJECT) ||
                    (left->getType() == JSValueType::ARRAY && right->getType() == JSValueType::ARRAY) ||
                    (left->getType() == JSValueType::FUNCTION && right->getType() == JSValueType::FUNCTION)) {
                    return std::make_shared<JSBoolean>(left.get() == right.get());
                }
                return std::make_shared<JSBoolean>(left->getType() == right->getType() && left->toString() == right->toString());
            case TokenType::NOT_EQUAL:
                // Simple NOT EQUAL implementation by negating EQUAL
                if (left->getType() == JSValueType::NULL_TYPE && right->getType() == JSValueType::UNDEFINED) return std::make_shared<JSBoolean>(false);
                if (left->getType() == JSValueType::UNDEFINED && right->getType() == JSValueType::NULL_TYPE) return std::make_shared<JSBoolean>(false);
                if (left->getType() != right->getType()) {
                    if ((left->getType() == JSValueType::NUMBER && right->getType() == JSValueType::STRING) ||
                        (left->getType() == JSValueType::STRING && right->getType() == JSValueType::NUMBER) ||
                        (left->getType() == JSValueType::BOOLEAN) || (right->getType() == JSValueType::BOOLEAN)) {
                        return std::make_shared<JSBoolean>(left->toNumber() != right->toNumber());
                    }
                }
            case TokenType::STRICT_NOT_EQUAL:
                if (left->getType() == JSValueType::NUMBER && right->getType() == JSValueType::NUMBER)
                    return std::make_shared<JSBoolean>(left->toNumber() != right->toNumber());
                if (left->getType() == JSValueType::STRING && right->getType() == JSValueType::STRING)
                    return std::make_shared<JSBoolean>(left->toString() != right->toString());
                if ((left->getType() == JSValueType::OBJECT && right->getType() == JSValueType::OBJECT) ||
                    (left->getType() == JSValueType::ARRAY && right->getType() == JSValueType::ARRAY) ||
                    (left->getType() == JSValueType::FUNCTION && right->getType() == JSValueType::FUNCTION)) {
                    return std::make_shared<JSBoolean>(left.get() != right.get());
                }
                return std::make_shared<JSBoolean>(left->getType() != right->getType() || left->toString() != right->toString());
            case TokenType::LESS:
                return std::make_shared<JSBoolean>(left->toNumber() < right->toNumber());
            case TokenType::LESS_EQUAL:
                return std::make_shared<JSBoolean>(left->toNumber() <= right->toNumber());
            case TokenType::GREATER:
                return std::make_shared<JSBoolean>(left->toNumber() > right->toNumber());
            case TokenType::GREATER_EQUAL:
                return std::make_shared<JSBoolean>(left->toNumber() >= right->toNumber());
            default:
                break;
        }
        return std::make_shared<JSUndefined>();
}

std::shared_ptr<JSValue> Evaluator::evalCallExpression(CallExpression* call) {
        class CallStackGuard {
            int& depth;
        public:
            CallStackGuard(int& d) : depth(d) {
                depth++;
                if (depth > 200) {
                    depth--;
                    throw RuntimeError("RangeError: Maximum call stack size exceeded");
                }
            }
            ~CallStackGuard() { depth--; }
        };
        CallStackGuard guard(callStackDepth);

        auto callee = evaluate(call->callee);
        
        if (std::dynamic_pointer_cast<SuperExpression>(call->callee)) {
            if (callee->getType() == JSValueType::OBJECT) {
                auto parentProto = std::dynamic_pointer_cast<JSObject>(callee);
                if (parentProto->properties.count("constructor")) {
                    callee = parentProto->properties["constructor"].value;
                } else {
                    throw RuntimeError("TypeError: Super constructor not found");
                }
            }
        }

        std::vector<std::shared_ptr<JSValue>> args;
        for (const auto& argExpr : call->arguments) {
            if (auto spread = std::dynamic_pointer_cast<SpreadElement>(argExpr)) {
                auto spreadVal = evaluate(spread->argument);
                if (spreadVal->getType() == JSValueType::ARRAY) {
                    auto spreadArr = std::dynamic_pointer_cast<JSArray>(spreadVal);
                    args.insert(args.end(), spreadArr->elements.begin(), spreadArr->elements.end());
                } else if (spreadVal->getType() == JSValueType::STRING) {
                    auto strVal = std::dynamic_pointer_cast<JSString>(spreadVal);
                    for (char c : strVal->value) args.push_back(std::make_shared<JSString>(std::string(1, c)));
                } else {
                    throw RuntimeError("TypeError: spread argument is not iterable");
                }
            } else {
                args.push_back(evaluate(argExpr));
            }
        }

        std::shared_ptr<JSValue> thisContext;
        if (std::dynamic_pointer_cast<SuperExpression>(call->callee)) {
            thisContext = environment->get("this");
        } else if (auto memberExpr = std::dynamic_pointer_cast<MemberExpression>(call->callee)) {
            thisContext = evaluate(memberExpr->object);
        } else {
            thisContext = std::make_shared<JSUndefined>();
        }

        return executeFunction(callee, thisContext, args);
}

std::shared_ptr<JSValue> Evaluator::evalMemberExpression(MemberExpression* member) {
        auto obj = evaluate(member->object);
        lastThisContext = obj;
        
        std::string propName;
        if (!member->computed) {
            if (auto strProp = std::dynamic_pointer_cast<StringLiteral>(member->property)) {
                propName = strProp->value;
            } else if (auto idProp = std::dynamic_pointer_cast<Identifier>(member->property)) {
                propName = idProp->name;
            }
        } else {
            propName = evaluate(member->property)->toString();
        }

        if (obj->getType() == JSValueType::ARRAY) {
            auto arr = std::dynamic_pointer_cast<JSArray>(obj);
            if (propName == "length") return std::make_shared<JSNumber>(arr->elements.size());
            auto method = getArrayMethod(arr, propName, [this](std::shared_ptr<JSFunction> func, const std::vector<std::shared_ptr<JSValue>>& args) {
                auto funcEnv = std::make_shared<Environment>(func->closure);
                size_t paramCount = func->declaration->parameters.size();
                for (size_t i = 0; i < paramCount; ++i) {
                    auto paramName = func->declaration->parameters[i].first;
                    auto defaultExpr = func->declaration->parameters[i].second;
                    if (func->declaration->hasRest && i == paramCount - 1) {
                        auto restArr = std::make_shared<JSArray>();
                        for (size_t j = i; j < args.size(); ++j) restArr->elements.push_back(args[j]);
                        funcEnv->define(paramName, restArr);
                    } else if (i < args.size() && args[i]->getType() != JSValueType::UNDEFINED) {
                        funcEnv->define(paramName, args[i]);
                    } else if (defaultExpr) {
                        funcEnv->define(paramName, evaluate(defaultExpr));
                    } else {
                        funcEnv->define(paramName, std::make_shared<JSUndefined>());
                    }
                }
                try {
                    for (const auto& s : func->declaration->body->statements) hoist(s);
                    executeBlock(func->declaration->body->statements, funcEnv);
                } catch (const ReturnException& ret) {
                    return ret.value;
                }
                return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
            });
            if (method->getType() != JSValueType::UNDEFINED) return method;
            
        } else if (obj->getType() == JSValueType::STRING) {
            auto str = std::dynamic_pointer_cast<JSString>(obj);
            if (propName == "length") return std::make_shared<JSNumber>(str->value.length());
            auto method = getStringMethod(str, propName);
            if (method->getType() != JSValueType::UNDEFINED) return method;
            
            try {
                auto stringObj = this->environment->get("String");
                if (stringObj->getType() == JSValueType::FUNCTION || stringObj->getType() == JSValueType::OBJECT) {
                    auto proto = stringObj->getType() == JSValueType::FUNCTION ? 
                        std::dynamic_pointer_cast<JSFunction>(stringObj)->prototypeProperty : 
                        std::dynamic_pointer_cast<JSObject>(stringObj)->properties["prototype"].value;
                    if (proto && proto->getType() == JSValueType::OBJECT) {
                        auto methodJS = std::dynamic_pointer_cast<JSObject>(proto)->properties[propName].value;
                        if (methodJS && (methodJS->getType() == JSValueType::FUNCTION || methodJS->getType() == JSValueType::NATIVE_FUNCTION)) {
                            return methodJS;
                        }
                    }
                }
            } catch(...) {}
            
            try {
                int idx = std::stoi(propName);
                if (idx >= 0 && idx < str->value.length()) return std::make_shared<JSString>(std::string(1, str->value[idx]));
            } catch (...) {}
            return std::make_shared<JSUndefined>();
        }

        if (obj->getType() == JSValueType::FUNCTION || obj->getType() == JSValueType::NATIVE_FUNCTION) {
            if (propName == "call") {
                return std::make_shared<JSNativeFunction>("call", [this, obj](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
                    std::shared_ptr<JSValue> thisCtx = std::make_shared<JSUndefined>();
                    if (!args.empty()) thisCtx = args[0];
                    std::vector<std::shared_ptr<JSValue>> callArgs;
                    if (args.size() > 1) {
                        for (size_t i = 1; i < args.size(); ++i) callArgs.push_back(args[i]);
                    }
                    return this->executeFunction(obj, thisCtx, callArgs);
                });
            }
            if (obj->getType() == JSValueType::FUNCTION) {
                auto funcObj = std::dynamic_pointer_cast<JSFunction>(obj);
                if (propName == "prototype") {
                    if (funcObj && funcObj->prototypeProperty) return funcObj->prototypeProperty;
                    return std::make_shared<JSUndefined>();
                }
                if (funcObj->declaration && funcObj->declaration->name == "Promise") {
                    if (propName == "resolve") {
                        return std::make_shared<JSNativeFunction>("resolve", [this](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
                            std::string code = "new Promise(function(resolve) { resolve(args_val); })";
                            Lexer lexer(code);
                            Parser parser(lexer.tokenize());
                            auto ast = parser.parse();
                            auto oldEnv = this->environment;
                            this->environment = std::make_shared<Environment>(oldEnv);
                            this->environment->define("args_val", args.empty() ? std::shared_ptr<JSValue>(std::make_shared<JSUndefined>()) : args[0]);
                            std::shared_ptr<JSValue> result = std::make_shared<JSUndefined>();
                            if (!ast->body.empty()) {
                                if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(ast->body[0])) {
                                    result = this->evaluate(exprStmt->expression);
                                }
                            }
                            this->environment = oldEnv;
                            return result;
                        });
                    }
                    if (propName == "reject") {
                        return std::make_shared<JSNativeFunction>("reject", [this](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
                            std::string code = "new Promise(function(resolve, reject) { reject(args_val); })";
                            Lexer lexer(code);
                            Parser parser(lexer.tokenize());
                            auto ast = parser.parse();
                            auto oldEnv = this->environment;
                            this->environment = std::make_shared<Environment>(oldEnv);
                            this->environment->define("args_val", args.empty() ? std::shared_ptr<JSValue>(std::make_shared<JSUndefined>()) : args[0]);
                            std::shared_ptr<JSValue> result = std::make_shared<JSUndefined>();
                            if (!ast->body.empty()) {
                                if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(ast->body[0])) {
                                    result = this->evaluate(exprStmt->expression);
                                }
                            }
                            this->environment = oldEnv;
                            return result;
                        });
                    }
                }
            }
            return std::make_shared<JSUndefined>();
        }
        
        if (obj->getType() == JSValueType::OBJECT || obj->getType() == JSValueType::ARRAY) {
            auto jsObj = std::dynamic_pointer_cast<JSObject>(obj);
            
            // Search in prototype chain
            auto currentObj = jsObj;
            while (currentObj) {
                if (currentObj->properties.count(propName)) {
                    auto& desc = currentObj->properties[propName];
                    if (desc.get) {
                        return executeFunction(desc.get, jsObj, {});
                    }
                    return desc.value ? desc.value : std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
                }
                currentObj = std::dynamic_pointer_cast<JSObject>(currentObj->prototype);
            }
            
            return std::make_shared<JSUndefined>();
        }
        throw RuntimeError("TypeError: Cannot read property '" + propName + "' of undefined or primitive");
}

std::shared_ptr<JSValue> Evaluator::evalNewExpression(NewExpression* newExpr) {
          if (auto id = std::dynamic_pointer_cast<Identifier>(newExpr->callee)) {
              if (id->name == "Date") {
                  auto dateObj = std::make_shared<JSObject>();
                  dateObj->properties["getTime"].value = std::make_shared<JSNativeFunction>("getTime", [](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
                      auto now = std::chrono::system_clock::now();
                      auto duration = now.time_since_epoch();
                      auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                      return std::make_shared<JSNumber>(millis);
                  });
                  return dateObj;
              }
          }
          auto callee = evaluate(newExpr->callee);
          std::vector<std::shared_ptr<JSValue>> args;
          for (const auto& argExpr : newExpr->arguments) {
            if (auto spread = std::dynamic_pointer_cast<SpreadElement>(argExpr)) {
                auto spreadVal = evaluate(spread->argument);
                if (spreadVal->getType() == JSValueType::ARRAY) {
                    auto spreadArr = std::dynamic_pointer_cast<JSArray>(spreadVal);
                    args.insert(args.end(), spreadArr->elements.begin(), spreadArr->elements.end());
                } else if (spreadVal->getType() == JSValueType::STRING) {
                    auto strVal = std::dynamic_pointer_cast<JSString>(spreadVal);
                    for (char c : strVal->value) args.push_back(std::make_shared<JSString>(std::string(1, c)));
                } else {
                    throw RuntimeError("TypeError: spread argument is not iterable");
                }
            } else {
                args.push_back(evaluate(argExpr));
            }
        }
        
        if (callee->getType() == JSValueType::NATIVE_FUNCTION) {
            auto nativeFunc = std::dynamic_pointer_cast<JSNativeFunction>(callee);
            return nativeFunc->func(args);
        }
        if (callee->getType() == JSValueType::FUNCTION) {
            auto func = std::dynamic_pointer_cast<JSFunction>(callee);
            
            auto newObj = std::make_shared<JSObject>();
            if (func->prototypeProperty) {
                newObj->prototype = std::dynamic_pointer_cast<JSObject>(func->prototypeProperty);
            }

            auto funcEnv = std::make_shared<Environment>(func->closure);
            funcEnv->define("this", newObj);

            size_t paramCount = func->declaration->parameters.size();
            for (size_t i = 0; i < paramCount; ++i) {
                auto paramName = func->declaration->parameters[i].first;
                auto defaultExpr = func->declaration->parameters[i].second;

                if (func->declaration->hasRest && i == paramCount - 1) {
                    auto restArr = std::make_shared<JSArray>();
                    for (size_t j = i; j < args.size(); ++j) restArr->elements.push_back(args[j]);
                    funcEnv->define(paramName, restArr);
                } else if (i < args.size() && args[i]->getType() != JSValueType::UNDEFINED) {
                    funcEnv->define(paramName, args[i]);
                } else if (defaultExpr) {
                    auto previousEnv = environment;
                    environment = funcEnv;
                    auto defVal = evaluate(defaultExpr);
                    environment = previousEnv;
                    funcEnv->define(paramName, defVal);
                } else {
                    funcEnv->define(paramName, std::make_shared<JSUndefined>());
                }
            }
            try {
                for (const auto& s : func->declaration->body->statements) hoist(s);
                executeBlock(func->declaration->body->statements, funcEnv);
            } catch (const ReturnException& ret) {
                if (ret.value->getType() == JSValueType::OBJECT || ret.value->getType() == JSValueType::ARRAY || ret.value->getType() == JSValueType::FUNCTION) {
                    return ret.value;
                }
            }
            return newObj;
        }
        throw RuntimeError("TypeError: callee is not a constructor");
}

std::shared_ptr<JSValue> Evaluator::evalSuperExpression(SuperExpression* superExpr) {
        auto thisObj = environment->get("this");
        if (thisObj->getType() == JSValueType::OBJECT) {
            auto jsObj = std::dynamic_pointer_cast<JSObject>(thisObj);
            if (jsObj->prototype && jsObj->prototype->getType() == JSValueType::OBJECT) {
                auto proto = std::dynamic_pointer_cast<JSObject>(jsObj->prototype);
                if (proto->prototype) {
                    return proto->prototype;
                }
            }
        }
        throw RuntimeError("TypeError: super property not found");
}

std::shared_ptr<JSValue> Evaluator::evaluate(std::shared_ptr<Expression> expr) {
    if (!expr) return std::make_shared<JSUndefined>();
    return expr->eval(this);
}

extern JSValue* gc_head;

void Evaluator::mark(std::shared_ptr<JSValue> value, std::unordered_set<std::shared_ptr<Environment>>& visitedEnv) {
    if (!value || value->gc_marked) return;
    value->gc_marked = true;
    
    if (value->getType() == JSValueType::OBJECT) {
        auto obj = std::dynamic_pointer_cast<JSObject>(value);
        if (obj) {
            for (auto& pair : obj->properties) {
                mark(pair.second.value, visitedEnv);
                if (pair.second.get) mark(pair.second.get, visitedEnv);
                if (pair.second.set) mark(pair.second.set, visitedEnv);
            }
        }
    } else if (value->getType() == JSValueType::ARRAY) {
        auto arr = std::dynamic_pointer_cast<JSArray>(value);
        if (arr) {
            for (auto& el : arr->elements) mark(el, visitedEnv);
        }
    } else if (value->getType() == JSValueType::FUNCTION) {
        auto func = std::dynamic_pointer_cast<JSFunction>(value);
        if (func) {
            mark(func->closure, visitedEnv);
            mark(func->prototypeProperty, visitedEnv);
        }
    }
}

void Evaluator::mark(std::shared_ptr<Environment> env, std::unordered_set<std::shared_ptr<Environment>>& visitedEnv) {
    if (!env || visitedEnv.count(env)) return;
    visitedEnv.insert(env);
    
    for (auto& pair : env->getLocalValues()) {
        mark(pair.second.value, visitedEnv);
    }
    mark(env->getEnclosing(), visitedEnv);
}

void Evaluator::markAndSweep() {
    // 1. Unmark all objects
    JSValue* curr = gc_head;
    while (curr) {
        curr->gc_marked = false;
        curr = curr->gc_next;
    }
    
    // 2. Mark from root environment and timer tasks/microtasks
    std::unordered_set<std::shared_ptr<Environment>> visitedEnv;
    mark(environment, visitedEnv);
    
    for (auto& task : timerQueue) {
        mark(task.callback, visitedEnv);
        for (auto& arg : task.args) mark(arg, visitedEnv);
    }
    // Note: microtasks use std::function which might capture Environment, but it's hard to trace. 
    // Usually microtasks are short-lived. We assume they keep things alive implicitly if we don't GC during microtasks.
    
    // 3. Sweep
    class JSDummyMarker : public JSValue {
    public:
        JSValueType getType() const override { return JSValueType::NULL_TYPE; }
        std::string toString() const override { return ""; }
        double toNumber() const override { return 0; }
        bool isTruthy() const override { return false; }
    };
    
    JSDummyMarker marker;
    // Remove marker from the list since it gets auto-inserted at head by JSValue()
    if (marker.gc_prev) marker.gc_prev->gc_next = marker.gc_next;
    else gc_head = marker.gc_next;
    if (marker.gc_next) marker.gc_next->gc_prev = marker.gc_prev;
    marker.gc_next = nullptr;
    marker.gc_prev = nullptr;

    curr = gc_head;
    int sweptCount = 0;
    std::vector<std::shared_ptr<JSValue>> dropped_values;
    std::vector<std::shared_ptr<Environment>> dropped_envs;
    while (curr) {
        // Insert marker after curr
        marker.gc_next = curr->gc_next;
        marker.gc_prev = curr;
        if (curr->gc_next) curr->gc_next->gc_prev = &marker;
        curr->gc_next = &marker;
        
        if (!curr->gc_marked) {
            if (curr->getType() == JSValueType::OBJECT) {
                auto obj = (JSObject*)curr;
                if (!obj->properties.empty()) { 
                    for (auto& pair : obj->properties) {
                        dropped_values.push_back(std::move(pair.second.value));
                        if (pair.second.get) dropped_values.push_back(std::move(pair.second.get));
                        if (pair.second.set) dropped_values.push_back(std::move(pair.second.set));
                    }
                    obj->properties.clear(); 
                    sweptCount++; 
                }
            } else if (curr->getType() == JSValueType::ARRAY) {
                auto arr = (JSArray*)curr;
                if (!arr->elements.empty()) { 
                    for (auto& el : arr->elements) dropped_values.push_back(std::move(el));
                    arr->elements.clear(); 
                    sweptCount++; 
                }
            } else if (curr->getType() == JSValueType::FUNCTION) {
                auto func = (JSFunction*)curr;
                if (func->closure || func->prototypeProperty) {
                    dropped_envs.push_back(std::move(func->closure));
                    dropped_values.push_back(std::move(func->prototypeProperty));
                    func->closure.reset();
                    func->prototypeProperty.reset();
                    sweptCount++;
                }
            }
        }
        
        // Advance curr to whatever is after the marker
        curr = marker.gc_next;
        
        // Remove marker
        if (marker.gc_prev) marker.gc_prev->gc_next = marker.gc_next;
        else gc_head = marker.gc_next;
        if (marker.gc_next) marker.gc_next->gc_prev = marker.gc_prev;
    }
}

void Evaluator::execContinueStatement(ContinueStatement* stmt) {
    throw ContinueException();
}
void Evaluator::execBreakStatement(BreakStatement* stmt) {
    throw BreakException();
}
std::shared_ptr<JSValue> Evaluator::evalThisExpression(ThisExpression* expr) {
    try {
        return environment->get("this");
    } catch (...) {
        return std::make_shared<JSUndefined>();
    }
}
std::shared_ptr<JSValue> Evaluator::evalSpreadElement(SpreadElement* expr) {
    throw RuntimeError("spread element not implemented directly");
}
