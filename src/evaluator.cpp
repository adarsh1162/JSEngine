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
        environment->assign(assign->name, value);
        return value;
    }
    if (auto binary = std::dynamic_pointer_cast<BinaryExpression>(expr)) {
        auto left = evaluate(binary->left);
        auto right = evaluate(binary->right);
        
        switch (binary->op) {
            case TokenType::PLUS:
                if (left->getType() == JSValueType::STRING || right->getType() == JSValueType::STRING) {
                    return std::make_shared<JSString>(left->toString() + right->toString());
                }
                return std::make_shared<JSNumber>(left->toNumber() + right->toNumber());
            case TokenType::MINUS: return std::make_shared<JSNumber>(left->toNumber() - right->toNumber());
            case TokenType::MULTIPLY: return std::make_shared<JSNumber>(left->toNumber() * right->toNumber());
            case TokenType::DIVIDE: return std::make_shared<JSNumber>(left->toNumber() / right->toNumber());
            case TokenType::MODULO: return std::make_shared<JSNumber>(std::fmod(left->toNumber(), right->toNumber()));
            case TokenType::POWER: return std::make_shared<JSNumber>(std::pow(left->toNumber(), right->toNumber()));
            case TokenType::STRICT_EQUAL:
            case TokenType::EQUAL:
                if (left->getType() == JSValueType::NUMBER && right->getType() == JSValueType::NUMBER)
                    return std::make_shared<JSBoolean>(left->toNumber() == right->toNumber());
                if (left->getType() == JSValueType::STRING && right->getType() == JSValueType::STRING)
                    return std::make_shared<JSBoolean>(left->toString() == right->toString());
                return std::make_shared<JSBoolean>(false);
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
            args.push_back(evaluate(argExpr));
        }

        if (callee->getType() == JSValueType::NATIVE_FUNCTION) {
            auto nativeFunc = std::dynamic_pointer_cast<JSNativeFunction>(callee);
            return nativeFunc->func(args);
        }
        if (callee->getType() == JSValueType::FUNCTION) {
            auto func = std::dynamic_pointer_cast<JSFunction>(callee);
            auto funcEnv = std::make_shared<Environment>(func->closure);
            for (size_t i = 0; i < func->declaration->parameters.size(); ++i) {
                if (i < args.size()) {
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
            auto method = getArrayMethod(arr, member->property);
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
    
    return std::make_shared<JSUndefined>();
}
