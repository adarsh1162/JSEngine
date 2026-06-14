#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <string>
#include <unordered_map>
#include <memory>
#include <stdexcept>
#include "types.h"

// Runtime Exception
class RuntimeError : public std::runtime_error {
public:
    explicit RuntimeError(const std::string& message) : std::runtime_error(message) {}
};

struct Variable {
    std::shared_ptr<JSValue> value;
    bool isConst;
    bool isTdz;
};

class Environment {
private:
    // Store variables: name -> Variable struct
    std::unordered_map<std::string, Variable> values;
    
    // Parent scope
    std::shared_ptr<Environment> enclosing;

public:
    // Global environment
    Environment();
    
    // Local (block/function) environment
    explicit Environment(std::shared_ptr<Environment> enclosingEnv, bool isFuncScope = false);

    bool isFunctionScope;

    // Create a new variable (`let` or `const`)
    void define(const std::string& name, std::shared_ptr<JSValue> value, bool isConst = false);

    // Define a variable in TDZ
    void defineTdz(const std::string& name, bool isConst = false);

    // Create a var variable (function scoped)
    void defineVar(const std::string& name, std::shared_ptr<JSValue> value);
    
    // Get a variable's value. If not in current, checks enclosing.
    std::shared_ptr<JSValue> get(const std::string& name);
    
    // Update an existing variable
    void assign(const std::string& name, std::shared_ptr<JSValue> value);

    // Get all local variables (for loop closure copying)
    const std::unordered_map<std::string, Variable>& getLocalValues() const {
        return values;
    }
    
    std::shared_ptr<Environment> getEnclosing() const {
        return enclosing;
    }
};

#endif // ENVIRONMENT_H
