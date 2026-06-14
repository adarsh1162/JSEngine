#include "environment.h"

Environment::Environment() : enclosing(nullptr), isFunctionScope(true) {}

Environment::Environment(std::shared_ptr<Environment> enclosingEnv, bool isFuncScope) : enclosing(enclosingEnv), isFunctionScope(isFuncScope) {}

void Environment::define(const std::string& name, std::shared_ptr<JSValue> value, bool isConst) {
    // In JS, let/const redefinition in same scope throws error, but for hackathon simplicity we allow overwrite
    values[name] = Variable{value, isConst, false};
}

void Environment::defineTdz(const std::string& name, bool isConst) {
    values[name] = Variable{std::make_shared<JSUndefined>(), isConst, true};
}

void Environment::defineVar(const std::string& name, std::shared_ptr<JSValue> value) {
    if (isFunctionScope || enclosing == nullptr) {
        values[name] = Variable{value, false, false};
    } else {
        enclosing->defineVar(name, value);
    }
}

std::shared_ptr<JSValue> Environment::get(const std::string& name) {
    auto it = values.find(name);
    if (it != values.end()) {
        if (it->second.isTdz) {
            throw RuntimeError("ReferenceError: Cannot access '" + name + "' before initialization");
        }
        return it->second.value;
    }

    if (enclosing != nullptr) {
        return enclosing->get(name);
    }

    throw RuntimeError("ReferenceError: " + name + " is not defined");
}

void Environment::assign(const std::string& name, std::shared_ptr<JSValue> value) {
    auto it = values.find(name);
    if (it != values.end()) {
        if (it->second.isTdz) {
            throw RuntimeError("ReferenceError: Cannot access '" + name + "' before initialization");
        }
        if (it->second.isConst) {
            throw RuntimeError("TypeError: Assignment to constant variable.");
        }
        it->second.value = value;
        return;
    }

    if (enclosing != nullptr) {
        enclosing->assign(name, value);
        return;
    }

    throw RuntimeError("ReferenceError: Cannot assign to undefined variable " + name);
}
