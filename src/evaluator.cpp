#include "evaluator.h"
#include "builtins.h"
#include <cmath>
#include <chrono>

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
}

void Evaluator::runEventLoop() {
    while (!timerQueue.empty()) {
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
        for (const auto& stmt : program->body) {
            execute(stmt);
        }
        runEventLoop();
    } catch (const JSException& e) {
        std::cerr << "Uncaught Error: " << e.value->toString() << std::endl;
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
        for (const auto& stmt : statements) {
            execute(stmt);
        }
        environment = previous;
    } catch (...) {
        environment = previous;
        throw;
    }
}


void Evaluator::execute(std::shared_ptr<Statement> stmt) {
    if (auto varDecl = std::dynamic_pointer_cast<VariableDeclaration>(stmt)) {
        std::shared_ptr<JSValue> value = std::make_shared<JSUndefined>();
        if (varDecl->initializer) {
            value = evaluate(varDecl->initializer);
        }
        environment->define(varDecl->name, value, varDecl->isConst);
    }
    else if (auto destDecl = std::dynamic_pointer_cast<DestructuringDeclaration>(stmt)) {
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
                        environment->define(key, obj->properties[key], destDecl->isConst);
                    } else {
                        environment->define(key, std::make_shared<JSUndefined>(), destDecl->isConst);
                    }
                }
            } else {
                throw RuntimeError("TypeError: Cannot destructure property of primitive");
            }
        }
    }
    else if (auto throwStmt = std::dynamic_pointer_cast<ThrowStatement>(stmt)) {
        throw JSException(evaluate(throwStmt->argument));
    }
    else if (auto tryStmt = std::dynamic_pointer_cast<TryStatement>(stmt)) {
        bool thrownJS = false, thrownReturn = false, thrownBreak = false, thrownContinue = false;
        std::shared_ptr<JSValue> returnValue = nullptr;
        std::shared_ptr<JSValue> exceptionValue = nullptr;

        try {
            executeBlock(tryStmt->block->statements, std::make_shared<Environment>(environment));
        } catch (const JSException& e) {
            if (tryStmt->handler) {
                auto catchEnv = std::make_shared<Environment>(environment);
                if (!tryStmt->param.empty()) {
                    catchEnv->define(tryStmt->param, e.value);
                }
                try {
                    executeBlock(tryStmt->handler->statements, catchEnv);
                } catch (const JSException& ce) {
                    thrownJS = true; exceptionValue = ce.value;
                } catch (const ReturnException& re) {
                    thrownReturn = true; returnValue = re.value;
                } catch (const BreakException&) {
                    thrownBreak = true;
                } catch (const ContinueException&) {
                    thrownContinue = true;
                }
            } else {
                thrownJS = true; exceptionValue = e.value;
            }
        } catch (const ReturnException& re) {
            thrownReturn = true; returnValue = re.value;
        } catch (const BreakException&) {
            thrownBreak = true;
        } catch (const ContinueException&) {
            thrownContinue = true;
        }

        if (tryStmt->finalizer) {
            executeBlock(tryStmt->finalizer->statements, std::make_shared<Environment>(environment));
        }

        if (thrownReturn) throw ReturnException(returnValue);
        if (thrownJS) throw JSException(exceptionValue);
        if (thrownBreak) throw BreakException();
        if (thrownContinue) throw ContinueException();
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
            } catch (const BreakException&) {
                break;
            } catch (const ContinueException&) {
                continue;
            } catch (const ReturnException&) {
                throw;
            }
        }
    }
    else if (auto doWhileStmt = std::dynamic_pointer_cast<DoWhileStatement>(stmt)) {
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
    else if (auto switchStmt = std::dynamic_pointer_cast<SwitchStatement>(stmt)) {
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
    else if (auto forStmt = std::dynamic_pointer_cast<ForStatement>(stmt)) {
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
    else if (auto forInStmt = std::dynamic_pointer_cast<ForInStatement>(stmt)) {
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
    else if (auto forOfStmt = std::dynamic_pointer_cast<ForOfStatement>(stmt)) {
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
            if (rightVal->getType() == JSValueType::ARRAY) {
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
    else if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(stmt)) {
        // Handled in hoist phase
    }
    else if (auto classDecl = std::dynamic_pointer_cast<ClassDeclaration>(stmt)) {
        std::shared_ptr<JSFunction> classConstructor;
        
        for (auto method : classDecl->methods) {
            if (method->name == "constructor") {
                auto decl = std::make_shared<FunctionDeclaration>("constructor", method->parameters, method->body);
                classConstructor = std::make_shared<JSFunction>(decl, environment);
                break;
            }
        }
        if (!classConstructor) {
            auto decl = std::make_shared<FunctionDeclaration>("constructor", std::vector<std::pair<std::string, std::shared_ptr<Expression>>>(), std::make_shared<BlockStatement>(std::vector<std::shared_ptr<Statement>>{}));
            classConstructor = std::make_shared<JSFunction>(decl, environment);
        }
        
        auto prototype = std::make_shared<JSObject>();
        if (classDecl->superClass) {
            auto superVal = evaluate(classDecl->superClass);
            if (superVal->getType() == JSValueType::FUNCTION) {
                auto superFunc = std::dynamic_pointer_cast<JSFunction>(superVal);
                prototype->prototype = std::dynamic_pointer_cast<JSObject>(superFunc->prototypeProperty); // Set __proto__
            } else {
                throw RuntimeError("TypeError: Super expression must be null or a function");
            }
        }
        
        for (auto method : classDecl->methods) {
            if (method->name == "constructor") continue;
            auto decl = std::make_shared<FunctionDeclaration>(method->name, method->parameters, method->body);
            auto methodFunc = std::make_shared<JSFunction>(decl, environment);
            prototype->properties[method->name] = methodFunc;
        }
        
        classConstructor->prototypeProperty = prototype;
        environment->define(classDecl->name, classConstructor);
    }
    else if (auto returnStmt = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
        std::shared_ptr<JSValue> value = std::make_shared<JSUndefined>();
        if (returnStmt->argument) {
            value = evaluate(returnStmt->argument);
        }
        throw ReturnException(value);
    }
    else if (std::dynamic_pointer_cast<BreakStatement>(stmt)) {
        throw BreakException();
    }
    else if (std::dynamic_pointer_cast<ContinueStatement>(stmt)) {
        throw ContinueException();
    }
    else if (auto throwStmt = std::dynamic_pointer_cast<ThrowStatement>(stmt)) {
        throw JSException(evaluate(throwStmt->argument));
    }
    else if (auto tryStmt = std::dynamic_pointer_cast<TryStatement>(stmt)) {
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
                } else {
                    throw RuntimeError("Spread argument is not iterable");
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
    if (auto tpl = std::dynamic_pointer_cast<TemplateLiteralExpression>(expr)) {
        std::string result = "";
        for (const auto& part : tpl->parts) {
            auto val = evaluate(part);
            result += val->toString();
        }
        return std::make_shared<JSString>(result);
    }
    if (auto regex = std::dynamic_pointer_cast<RegexLiteralExpression>(expr)) {
        auto obj = std::make_shared<JSObject>();
        obj->properties["source"] = std::make_shared<JSString>(regex->pattern);
        obj->properties["flags"] = std::make_shared<JSString>(regex->flags);
        return obj;
    }
    if (auto condExpr = std::dynamic_pointer_cast<ConditionalExpression>(expr)) {
        auto testVal = evaluate(condExpr->test);
        return testVal->isTruthy() ? evaluate(condExpr->consequent) : evaluate(condExpr->alternate);
    }
    if (auto assign = std::dynamic_pointer_cast<AssignmentExpression>(expr)) {
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
                auto jsObj = std::dynamic_pointer_cast<JSObject>(obj);
                if (jsObj->properties.count(propName)) current = jsObj->properties[propName];
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
                auto jsObj = std::dynamic_pointer_cast<JSObject>(obj);
                jsObj->properties[propName] = value;
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
    if (auto update = std::dynamic_pointer_cast<UpdateExpression>(expr)) {
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
                if (targetObj->properties.count(propName)) current = targetObj->properties[propName];
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
            targetObj->properties[propName] = newJsVal;
        } else if (targetArr && arrIdx >= 0) {
            while (arrIdx >= targetArr->elements.size()) targetArr->elements.push_back(std::make_shared<JSUndefined>());
            targetArr->elements[arrIdx] = newJsVal;
        }
        
        return update->isPrefix ? newJsVal : std::make_shared<JSNumber>(oldVal);
    }
    if (auto unary = std::dynamic_pointer_cast<UnaryExpression>(expr)) {
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
    }
    if (auto binary = std::dynamic_pointer_cast<BinaryExpression>(expr)) {
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
            case TokenType::BITWISE_AND: return std::make_shared<JSNumber>(static_cast<int>(left->toNumber()) & static_cast<int>(right->toNumber()));
            case TokenType::BITWISE_OR: return std::make_shared<JSNumber>(static_cast<int>(left->toNumber()) | static_cast<int>(right->toNumber()));
            case TokenType::BITWISE_XOR: return std::make_shared<JSNumber>(static_cast<int>(left->toNumber()) ^ static_cast<int>(right->toNumber()));
            case TokenType::LEFT_SHIFT: return std::make_shared<JSNumber>(static_cast<int>(left->toNumber()) << static_cast<int>(right->toNumber()));
            case TokenType::RIGHT_SHIFT: return std::make_shared<JSNumber>(static_cast<int>(left->toNumber()) >> static_cast<int>(right->toNumber()));
            case TokenType::UNSIGNED_RIGHT_SHIFT: return std::make_shared<JSNumber>(static_cast<unsigned int>(left->toNumber()) >> static_cast<unsigned int>(right->toNumber()));
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
    }
    if (auto call = std::dynamic_pointer_cast<CallExpression>(expr)) {
        auto callee = evaluate(call->callee);
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

        if (callee->getType() == JSValueType::NATIVE_FUNCTION) {
            auto nativeFunc = std::dynamic_pointer_cast<JSNativeFunction>(callee);
            return nativeFunc->func(args);
        }
        if (callee->getType() == JSValueType::FUNCTION) {
            auto func = std::dynamic_pointer_cast<JSFunction>(callee);
            
            auto funcEnv = std::make_shared<Environment>(func->closure);
            
            if (auto memberExpr = std::dynamic_pointer_cast<MemberExpression>(call->callee)) {
                auto thisObj = evaluate(memberExpr->object);
                funcEnv->define("this", thisObj);
            } else {
                funcEnv->define("this", std::make_shared<JSUndefined>());
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
    if (auto member = std::dynamic_pointer_cast<MemberExpression>(expr)) {
        auto obj = evaluate(member->object);
        
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
            
            try {
                int idx = std::stoi(propName);
                if (idx >= 0 && idx < arr->elements.size()) return arr->elements[idx];
            } catch (...) {}
            return std::make_shared<JSUndefined>();
        }
        else if (obj->getType() == JSValueType::STRING) {
            auto str = std::dynamic_pointer_cast<JSString>(obj);
            if (propName == "length") return std::make_shared<JSNumber>(str->value.length());
            auto method = getStringMethod(str, propName);
            if (method->getType() != JSValueType::UNDEFINED) return method;
            try {
                int idx = std::stoi(propName);
                if (idx >= 0 && idx < str->value.length()) return std::make_shared<JSString>(std::string(1, str->value[idx]));
            } catch (...) {}
            return std::make_shared<JSUndefined>();
        }

        if (obj->getType() == JSValueType::OBJECT || obj->getType() == JSValueType::ARRAY) {
            auto jsObj = std::dynamic_pointer_cast<JSObject>(obj);
            
            // Search in prototype chain
            auto currentObj = jsObj;
            while (currentObj) {
                if (currentObj->properties.count(propName)) {
                    return currentObj->properties[propName];
                }
                currentObj = std::dynamic_pointer_cast<JSObject>(currentObj->prototype);
            }
            
            return std::make_shared<JSUndefined>();
        }
        throw RuntimeError("TypeError: Cannot read property '" + propName + "' of undefined or primitive");
    }
    if (auto newExpr = std::dynamic_pointer_cast<NewExpression>(expr)) {
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

        if (auto id = std::dynamic_pointer_cast<Identifier>(newExpr->callee)) {
            if (id->name == "Date") {
                auto dateObj = std::make_shared<JSObject>();
                dateObj->properties["getTime"] = std::make_shared<JSNativeFunction>("getTime", [](const std::vector<std::shared_ptr<JSValue>>& args) {
                    auto now = std::chrono::system_clock::now();
                    auto duration = now.time_since_epoch();
                    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                    return std::make_shared<JSNumber>(millis);
                });
                return dateObj;
            }
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
    if (auto superExpr = std::dynamic_pointer_cast<SuperExpression>(expr)) {
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
    if (std::dynamic_pointer_cast<ThisExpression>(expr)) {
        try {
            return environment->get("this");
        } catch (...) {
            // Global scope 'this' is undefined in strict mode
            return std::make_shared<JSUndefined>();
        }
    }
    
    return std::make_shared<JSUndefined>();
}
