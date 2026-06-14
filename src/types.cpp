#include "types.h"
#include <sstream>
JSValue* gc_head = nullptr;

JSValue::JSValue() {
    gc_next = gc_head;
    if (gc_head) gc_head->gc_prev = this;
    gc_head = this;
}

JSValue::~JSValue() {
    if (gc_prev) gc_prev->gc_next = gc_next;
    else gc_head = gc_next;
    if (gc_next) gc_next->gc_prev = gc_prev;
}


std::string JSNumber::toString() const {
    // To handle JS number stringification cleanly (e.g. 7.0 -> "7")
    std::ostringstream out;
    out << value;
    return out.str();
}

double JSString::toNumber() const {
    try {
        if (value.empty()) return 0.0;
        size_t idx;
        double d = std::stod(value, &idx);
        // If not the entire string is consumed, it's typically NaN in JS unless trailing spaces
        // But for simplicity in this hackathon, stod is close enough
        return d;
    } catch (...) {
        return std::numeric_limits<double>::quiet_NaN();
    }
}

#include "ast.h"
#include "environment.h"

JSFunction::JSFunction(std::shared_ptr<FunctionDeclaration> decl, std::shared_ptr<Environment> env) 
    : declaration(decl), closure(env) {
        prototypeProperty = std::make_shared<JSObject>();
    }

std::string JSFunction::toString() const {
    if (declaration && !declaration->name.empty()) {
        return "function " + declaration->name + "() { ... }";
    }
    return "function() { ... }";
}
