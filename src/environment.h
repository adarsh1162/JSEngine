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

class Environment {
private:
    // Store variables: name -> JSValue
    std::unordered_map<std::string, std::shared_ptr<JSValue>> values;
    
    // Parent scope
    std::shared_ptr<Environment> enclosing;

public:
    // Global environment
    Environment();
    
    // Local (block/function) environment
    explicit Environment(std::shared_ptr<Environment> enclosingEnv);

    // Create a new variable (`let` or `const`)
    void define(const std::string& name, std::shared_ptr<JSValue> value);
    
    // Get a variable's value. If not in current, checks enclosing.
    std::shared_ptr<JSValue> get(const std::string& name);
    
    // Update an existing variable
    void assign(const std::string& name, std::shared_ptr<JSValue> value);
};

#endif // ENVIRONMENT_H
