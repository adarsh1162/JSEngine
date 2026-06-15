#ifndef EVALUATOR_H
#define EVALUATOR_H

#include <memory>
#include <vector>
#include <queue>
#include <functional>
#include <iostream>
#include "ast.h"
#include "environment.h"
#include "types.h"

// Special exception to handle JS "return" statement unwinding the C++ call stack
class ReturnException : public std::exception {
public:
    std::shared_ptr<JSValue> value;
    explicit ReturnException(std::shared_ptr<JSValue> val) : value(val) {}
};

class JSException : public std::exception {
public:
    std::shared_ptr<JSValue> value;
    explicit JSException(std::shared_ptr<JSValue> val) : value(val) {}
};

class BreakException : public std::exception {
public:
    BreakException() = default;
    const char* what() const noexcept override { return "Break exception"; }
};

class ContinueException : public std::exception {
public:
    ContinueException() = default;
    const char* what() const noexcept override { return "Continue exception"; }
};

#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <algorithm>

struct TimerTask {
    long long triggerTimeMs;
    std::shared_ptr<JSValue> callback; // Can be JSFunction or NativeFunction
    std::vector<std::shared_ptr<JSValue>> args;
    int id;
    bool isInterval;
    long long intervalMs;
    bool cancelled = false;
};

class Evaluator {
// Visitor Pattern AST Handlers
public:
    void execContinueStatement(ContinueStatement* stmt);
    void execExpressionStatement(ExpressionStatement* stmt);
    void execThrowStatement(ThrowStatement* stmt);
    void execReturnStatement(ReturnStatement* stmt);
    void execBlockStatement(BlockStatement* stmt);
    void execIfStatement(IfStatement* stmt);
    void execDoWhileStatement(DoWhileStatement* stmt);
    void execForOfStatement(ForOfStatement* stmt);
    void execSwitchStatement(SwitchStatement* stmt);
    void execDestructuringDeclaration(DestructuringDeclaration* stmt);
    void execClassDeclaration(ClassDeclaration* stmt);
    void execTryStatement(TryStatement* stmt);
    void execWhileStatement(WhileStatement* stmt);
    void execBreakStatement(BreakStatement* stmt);
    void execFunctionDeclaration(FunctionDeclaration* stmt);
    void execForStatement(ForStatement* stmt);
    void execVariableDeclaration(VariableDeclaration* stmt);
    void execForInStatement(ForInStatement* stmt);
    std::shared_ptr<JSValue> evalArrayLiteral(ArrayLiteral* expr);
    std::shared_ptr<JSValue> evalNumberLiteral(NumberLiteral* expr);
    std::shared_ptr<JSValue> evalThisExpression(ThisExpression* expr);
    std::shared_ptr<JSValue> evalUnaryExpression(UnaryExpression* expr);
    std::shared_ptr<JSValue> evalBinaryExpression(BinaryExpression* expr);
    std::shared_ptr<JSValue> evalTemplateLiteralExpression(TemplateLiteralExpression* expr);
    std::shared_ptr<JSValue> evalObjectLiteral(ObjectLiteral* expr);
    std::shared_ptr<JSValue> evalBooleanLiteral(BooleanLiteral* expr);
    std::shared_ptr<JSValue> evalConditionalExpression(ConditionalExpression* expr);
    std::shared_ptr<JSValue> evalRegexLiteralExpression(RegexLiteralExpression* expr);
    std::shared_ptr<JSValue> evalNewExpression(NewExpression* expr);
    std::shared_ptr<JSValue> evalAssignmentExpression(AssignmentExpression* expr);
    std::shared_ptr<JSValue> evalMemberExpression(MemberExpression* expr);
    std::shared_ptr<JSValue> evalFunctionExpression(FunctionExpression* expr);
    std::shared_ptr<JSValue> evalIdentifier(Identifier* expr);
    std::shared_ptr<JSValue> evalArrowFunctionExpression(ArrowFunctionExpression* expr);
    std::shared_ptr<JSValue> evalCallExpression(CallExpression* expr);
    std::shared_ptr<JSValue> evalUpdateExpression(UpdateExpression* expr);
    std::shared_ptr<JSValue> evalSpreadElement(SpreadElement* expr);
    std::shared_ptr<JSValue> evalSuperExpression(SuperExpression* expr);
    std::shared_ptr<JSValue> evalStringLiteral(StringLiteral* expr);

private:
    int callStackDepth = 0;
    std::vector<std::string> executionStack;
    int eventLoopTick = 0;
    std::shared_ptr<JSValue> lastThisContext;

public:
    std::shared_ptr<Environment> environment;
    std::shared_ptr<JSObject> objectPrototype;
    std::shared_ptr<JSObject> arrayPrototype;
    std::vector<TimerTask> timerQueue;
    std::unordered_set<int> cancelledTimers;
    int nextTimerId = 1;
    
    std::queue<std::function<void()>> microtaskQueue;
    void runMicrotasks();
    void enqueueMicrotask(std::function<void()> task) { microtaskQueue.push(task); }
    
    void runEventLoop();

    // Module Resolution Cache
    std::unordered_map<std::string, std::shared_ptr<JSValue>> requireCache;
    std::string resolveModule(const std::string& requestPath, const std::string& currentDir);
    std::shared_ptr<JSValue> createRequireFunction(const std::string& currentDir);

    // Helpers
    std::shared_ptr<JSValue> evaluate(std::shared_ptr<Expression> expr);
    std::shared_ptr<JSValue> executeFunction(std::shared_ptr<JSValue> funcVal, std::shared_ptr<JSValue> thisContext, const std::vector<std::shared_ptr<JSValue>>& args);
    void execute(std::shared_ptr<Statement> stmt);
    void executeBlock(const std::vector<std::shared_ptr<Statement>>& statements, std::shared_ptr<Environment> env);
    void registerTDZ(const std::vector<std::shared_ptr<Statement>>& statements, std::shared_ptr<Environment> env);
    void hoist(std::shared_ptr<Statement> stmt);

    // Garbage Collection
    void mark(std::shared_ptr<JSValue> value, std::unordered_set<std::shared_ptr<Environment>>& visitedEnv);
    void mark(std::shared_ptr<Environment> env, std::unordered_set<std::shared_ptr<Environment>>& visitedEnv);
    void markAndSweep();

    // Builtins
    void setupGlobalEnvironment();

public:
    Evaluator();
    void interpret(std::shared_ptr<Program> program);
};

#endif // EVALUATOR_H
