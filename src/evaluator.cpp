#include "evaluator.h"
#include "builtins.h"
#include <cmath>

// --- Builtins ---
std::shared_ptr<JSValue> builtin_console_log(const std::vector<std::shared_ptr<JSValue>>& args) {
    for (size_t i = 0; i < args.size(); ++i) {
        std::cout << args[i]->toString();
        if (i < args.size() - 1) std::cout << " ";
    }
    std::cout << std::endl;
    return std::make_shared<JSUndefined>();
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
    auto consoleObj = std::make_shared<JSObject>();
    consoleObj->properties["log"] = std::make_shared<JSNativeFunction>("log", builtin_console_log);
    environment->define("console", consoleObj);

    auto mathObj = std::make_shared<JSObject>();
    mathObj->properties["floor"] = std::make_shared<JSNativeFunction>("floor", builtin_math_floor);
    mathObj->properties["random"] = std::make_shared<JSNativeFunction>("random", [](const std::vector<std::shared_ptr<JSValue>>& args) {
        return std::make_shared<JSNumber>((double)rand() / RAND_MAX);
    });
    mathObj->properties["max"] = std::make_shared<JSNativeFunction>("max", [](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(-INFINITY));
        double maxVal = args[0]->toNumber();
        for (const auto& a : args) maxVal = std::max(maxVal, a->toNumber());
        return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(maxVal));
    });
    mathObj->properties["min"] = std::make_shared<JSNativeFunction>("min", [](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(INFINITY));
        double minVal = args[0]->toNumber();
        for (const auto& a : args) minVal = std::min(minVal, a->toNumber());
        return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(minVal));
    });
    mathObj->properties["pow"] = std::make_shared<JSNativeFunction>("pow", [](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.size() < 2) return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(NAN));
        return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(std::pow(args[0]->toNumber(), args[1]->toNumber())));
    });
    mathObj->properties["round"] = std::make_shared<JSNativeFunction>("round", [](const std::vector<std::shared_ptr<JSValue>>& args) {
        if (args.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(NAN));
        return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(std::round(args[0]->toNumber())));
    });
    environment->define("Math", mathObj);
}

void Evaluator::interpret(std::shared_ptr<Program> program) {
    try {
        for (const auto& stmt : program->body) {
            execute(stmt);
        }
    } catch (const RuntimeError& e) {
        std::cerr << e.what() << std::endl;
    } catch (const ReturnException& e) {
        // Uncaught return at top level (ignored)
    }
}

void Evaluator::execute(std::shared_ptr<Statement> stmt) {
    if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(stmt)) {
        std::shared_ptr<JSValue> value = std::make_shared<JSUndefined>();
        if (varDecl->initializer) {
            value = evaluate(varDecl->initializer);
        }
        environment->define(varDecl->name, value);
    }
    else if (auto exprStmt = std::dynamic_pointer_cast<ExpressionStatement>(stmt)) {
        evaluate(exprStmt->expression);
    }
    else if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
        auto condition = evaluate(ifStmt->condition);
        if (condition->isTruthy()) {
            execute(ifStmt->consequent);
        } else if (ifStmt->alternate) {
            execute(ifStmt->alternate);
        }
    }
    else if (auto blockStmt = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
        auto blockEnv = std::make_shared<Environment>(environment);
        executeBlock(blockStmt->statements, blockEnv);
    }
    else if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
        while (evaluate(whileStmt->condition)->isTruthy()) {
            try {
                execute(whileStmt->body);
            } catch (const ReturnException&) {
                throw; // Propagate return upwards
            }
        }
    }
    else if (auto forStmt = std::dynamic_pointer_cast<ForStatement>(stmt)) {
        // Create a new scope for the init variable
        auto forEnv = std::make_shared<Environment>(environment);
        auto previous = environment;
        environment = forEnv;

        try {
            if (forStmt->init) {
                execute(forStmt->init);
            }
            while (forStmt->condition ? evaluate(forStmt->condition)->isTruthy() : true) {
                try {
                    execute(forStmt->body);
                } catch (const ReturnException&) {
                    throw;
                }
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
    else if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(stmt)) {
        auto function = std::make_shared<JSFunction>(funcDecl, environment);
        environment->define(funcDecl->name, function);
    }
    else if (auto returnStmt = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
        std::shared_ptr<JSValue> value = std::make_shared<JSUndefined>();
        if (returnStmt->argument) {
            value = evaluate(returnStmt->argument);
        }
        throw ReturnException(value);
    }
}

void Evaluator::executeBlock(const std::vector<std::shared_ptr<Statement>>& statements, std::shared_ptr<Environment> env) {
    auto previous = environment;
    try {
        environment = env;
        for (const auto& stmt : statements) {
            execute(stmt);
        }
        environment = previous;
    } catch (...) {
        environment = previous;
        throw;
    }
}

std::shared_ptr<JSValue> Evaluator::evaluate(std::shared_ptr<Expression> expr) {
    if (auto num = std::dynamic_pointer_cast<NumberLiteral>(expr)) {
        return std::make_shared<JSNumber>(num->value);
    }
    if (auto str = std::dynamic_pointer_cast<StringLiteral>(expr)) {
        return std::make_shared<JSString>(str->value);
    }
    if (auto boolLit = std::dynamic_pointer_cast<BooleanLiteral>(expr)) {
        return std::make_shared<JSBoolean>(boolLit->value);
    }
    if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
        if (id->name == "undefined") return std::make_shared<JSUndefined>();
        if (id->name == "null") return std::make_shared<JSNull>();
        return environment->get(id->name);
    }
    if (auto arrLit = std::dynamic_pointer_cast<ArrayLiteral>(expr)) {
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
                }
            } else {
                arr->elements.push_back(evaluate(elem));
            }
        }
        return arr;
    }
    if (auto objLit = std::dynamic_pointer_cast<ObjectLiteral>(expr)) {
        auto obj = std::make_shared<JSObject>();
        for (const auto& prop : objLit->properties) {
            obj->properties[prop.key] = evaluate(prop.value);
        }
        return obj;
    }
    if (auto funcExpr = std::dynamic_pointer_cast<FunctionExpression>(expr)) {
        auto decl = std::make_shared<FunctionDeclaration>(funcExpr->name, funcExpr->parameters, funcExpr->body);
        return std::make_shared<JSFunction>(decl, environment);
    }
    if (auto arrowExpr = std::dynamic_pointer_cast<ArrowFunctionExpression>(expr)) {
        auto decl = std::make_shared<FunctionDeclaration>("", arrowExpr->parameters, arrowExpr->body);
        return std::make_shared<JSFunction>(decl, environment);
    }
    if (auto assign = std::dynamic_pointer_cast<AssignmentExpression>(expr)) {
        auto value = evaluate(assign->value);
        if (assign->op != TokenType::ASSIGN) {
            auto current = environment->get(assign->name);
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
        environment->assign(assign->name, value);
        return value;
    }
    if (auto unary = std::dynamic_pointer_cast<UnaryExpression>(expr)) {
        auto arg = evaluate(unary->argument);
        if (unary->op == TokenType::MINUS) {
            return std::make_shared<JSNumber>(-arg->toNumber());
        } else if (unary->op == TokenType::LOGICAL_NOT) {
            return std::make_shared<JSBoolean>(!arg->isTruthy());
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
    }
    if (auto binary = std::dynamic_pointer_cast<BinaryExpression>(expr)) {
        auto left = evaluate(binary->left);
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
            case TokenType::EQUAL:
                if (left->getType() == JSValueType::NULL_TYPE && right->getType() == JSValueType::UNDEFINED) return std::make_shared<JSBoolean>(true);
                if (left->getType() == JSValueType::UNDEFINED && right->getType() == JSValueType::NULL_TYPE) return std::make_shared<JSBoolean>(true);
            case TokenType::STRICT_EQUAL:
                if (left->getType() == JSValueType::NUMBER && right->getType() == JSValueType::NUMBER)
                    return std::make_shared<JSBoolean>(left->toNumber() == right->toNumber());
                if (left->getType() == JSValueType::STRING && right->getType() == JSValueType::STRING)
                    return std::make_shared<JSBoolean>(left->toString() == right->toString());
                return std::make_shared<JSBoolean>(left->getType() == right->getType() && left->toString() == right->toString());
            case TokenType::LESS:
                return std::make_shared<JSBoolean>(left->toNumber() < right->toNumber());
            case TokenType::LESS_EQUAL:
                return std::make_shared<JSBoolean>(left->toNumber() <= right->toNumber());
            case TokenType::GREATER:
                return std::make_shared<JSBoolean>(left->toNumber() > right->toNumber());
            case TokenType::GREATER_EQUAL:
                return std::make_shared<JSBoolean>(left->toNumber() >= right->toNumber());
            case TokenType::LOGICAL_AND:
                return left->isTruthy() ? right : left;
            case TokenType::LOGICAL_OR:
                return left->isTruthy() ? left : right;
            default:
                break;
        }
    }
    if (auto call = std::dynamic_pointer_cast<CallExpression>(expr)) {
        auto callee = evaluate(call->callee);
        std::vector<std::shared_ptr<JSValue>> args;
        for (const auto& argExpr : call->arguments) {
            args.push_back(evaluate(argExpr));
        }

        if (callee->getType() == JSValueType::NATIVE_FUNCTION) {
            auto nativeFunc = std::dynamic_pointer_cast<JSNativeFunction>(callee);
            return nativeFunc->func(args);
        }
        if (callee->getType() == JSValueType::FUNCTION) {
            auto func = std::dynamic_pointer_cast<JSFunction>(callee);
            auto funcEnv = std::make_shared<Environment>(func->closure);
            
            size_t paramCount = func->declaration->parameters.size();
            for (size_t i = 0; i < paramCount; ++i) {
                if (func->declaration->hasRest && i == paramCount - 1) {
                    auto restArr = std::make_shared<JSArray>();
                    for (size_t j = i; j < args.size(); ++j) restArr->elements.push_back(args[j]);
                    funcEnv->define(func->declaration->parameters[i], restArr);
                } else if (i < args.size()) {
                    funcEnv->define(func->declaration->parameters[i], args[i]);
                } else {
                    funcEnv->define(func->declaration->parameters[i], std::make_shared<JSUndefined>());
                }
            }
            try {
                executeBlock(func->declaration->body->statements, funcEnv);
            } catch (const ReturnException& ret) {
                return ret.value;
            }
            return std::make_shared<JSUndefined>();
        }
        throw RuntimeError("TypeError: callee is not a function");
    }
    if (auto member = std::dynamic_pointer_cast<MemberExpression>(expr)) {
        auto obj = evaluate(member->object);
        
        if (obj->getType() == JSValueType::ARRAY) {
            auto arr = std::dynamic_pointer_cast<JSArray>(obj);
            auto method = getArrayMethod(arr, member->property, [this](std::shared_ptr<JSFunction> func, const std::vector<std::shared_ptr<JSValue>>& args) {
                auto funcEnv = std::make_shared<Environment>(func->closure);
                size_t paramCount = func->declaration->parameters.size();
                for (size_t i = 0; i < paramCount; ++i) {
                    if (func->declaration->hasRest && i == paramCount - 1) {
                        auto restArr = std::make_shared<JSArray>();
                        for (size_t j = i; j < args.size(); ++j) restArr->elements.push_back(args[j]);
                        funcEnv->define(func->declaration->parameters[i], restArr);
                    } else if (i < args.size()) {
                        funcEnv->define(func->declaration->parameters[i], args[i]);
                    } else {
                        funcEnv->define(func->declaration->parameters[i], std::make_shared<JSUndefined>());
                    }
                }
                try {
                    executeBlock(func->declaration->body->statements, funcEnv);
                } catch (const ReturnException& ret) {
                    return ret.value;
                }
                return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
            });
            if (method->getType() != JSValueType::UNDEFINED) return method;
        }
        else if (obj->getType() == JSValueType::STRING) {
            auto str = std::dynamic_pointer_cast<JSString>(obj);
            auto method = getStringMethod(str, member->property);
            if (method->getType() != JSValueType::UNDEFINED) return method;
            if (member->property == "length") {
                return std::make_shared<JSNumber>(str->value.length());
            }
        }

        if (obj->getType() == JSValueType::OBJECT || obj->getType() == JSValueType::ARRAY) {
            auto jsObj = std::dynamic_pointer_cast<JSObject>(obj);
            if (jsObj->properties.count(member->property)) {
                return jsObj->properties[member->property];
            }
            return std::make_shared<JSUndefined>();
        }
        throw RuntimeError("TypeError: Cannot read property '" + member->property + "' of undefined or primitive");
    }
    
    if (auto newExpr = std::dynamic_pointer_cast<NewExpression>(expr)) {
        if (auto id = std::dynamic_pointer_cast<Identifier>(newExpr->callee)) {
            if (id->name == "Date") {
                auto dateObj = std::make_shared<JSObject>();
                dateObj->properties["getTime"] = std::make_shared<JSNativeFunction>("getTime", [](const std::vector<std::shared_ptr<JSValue>>& args) {
                    return std::make_shared<JSNumber>(1622540800000); // Mock timestamp
                });
                return dateObj;
            }
        }
        return std::make_shared<JSUndefined>();
    }
    
    return std::make_shared<JSUndefined>();
}
