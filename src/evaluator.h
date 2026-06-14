#ifndef EVALUATOR_H
#define EVALUATOR_H

#include <memory>
#include <vector>
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
private:
    std::shared_ptr<Environment> environment;
    int callStackDepth = 0;
    std::shared_ptr<JSObject> objectPrototype;
    std::shared_ptr<JSValue> lastThisContext;

public:
    std::vector<TimerTask> timerQueue;
    std::unordered_set<int> cancelledTimers;
    int nextTimerId = 1;
    void runEventLoop();

    // Helpers
    std::shared_ptr<JSValue> evaluate(std::shared_ptr<Expression> expr);
    void execute(std::shared_ptr<Statement> stmt);
    void executeBlock(const std::vector<std::shared_ptr<Statement>>& statements, std::shared_ptr<Environment> env);
    void registerTDZ(const std::vector<std::shared_ptr<Statement>>& statements, std::shared_ptr<Environment> env);
    void hoist(std::shared_ptr<Statement> stmt);

    // Builtins
    void setupGlobalEnvironment();

public:
    Evaluator();
    void interpret(std::shared_ptr<Program> program);
};

#endif // EVALUATOR_H
