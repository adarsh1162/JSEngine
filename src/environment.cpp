#include "environment.h"

Environment::Environment() : enclosing(nullptr) {}

Environment::Environment(std::shared_ptr<Environment> enclosingEnv) : enclosing(enclosingEnv) {}

void Environment::define(const std::string& name, std::shared_ptr<JSValue> value) {
    // In JS, let/const redefinition in same scope throws error, but for hackathon simplicity we allow overwrite
    values[name] = value;
}

std::shared_ptr<JSValue> Environment::get(const std::string& name) {
    auto it = values.find(name);
    if (it != values.end()) {
        return it->second;
    }

    if (enclosing != nullptr) {
        return enclosing->get(name);
    }

    throw RuntimeError("ReferenceError: " + name + " is not defined");
}

void Environment::assign(const std::string& name, std::shared_ptr<JSValue> value) {
    auto it = values.find(name);
    if (it != values.end()) {
        it->second = value;
        return;
    }

    if (enclosing != nullptr) {
        enclosing->assign(name, value);
        return;
    }

    throw RuntimeError("ReferenceError: Cannot assign to undefined variable " + name);
}
