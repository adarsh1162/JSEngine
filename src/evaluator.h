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

class Evaluator {
private:
    std::shared_ptr<Environment> environment;
    
    // Helpers
    std::shared_ptr<JSValue> evaluate(std::shared_ptr<Expression> expr);
    void execute(std::shared_ptr<Statement> stmt);
    void executeBlock(const std::vector<std::shared_ptr<Statement>>& statements, std::shared_ptr<Environment> env);

    // Builtins
    void setupGlobalEnvironment();

public:
    Evaluator();
    void interpret(std::shared_ptr<Program> program);
};

#endif // EVALUATOR_H
