#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <limits>

enum class JSValueType { NUMBER, STRING, BOOLEAN, NULL_TYPE, UNDEFINED, OBJECT, ARRAY, FUNCTION, NATIVE_FUNCTION };

class JSValue {
public:
    virtual ~JSValue() = default;
    virtual JSValueType getType() const = 0;
    virtual std::string toString() const = 0;
    virtual double toNumber() const = 0;
    virtual bool isTruthy() const = 0;
};

class JSNumber : public JSValue {
public:
    double value;
    explicit JSNumber(double v) : value(v) {}
    JSValueType getType() const override { return JSValueType::NUMBER; }
    std::string toString() const override;
    double toNumber() const override { return value; }
    bool isTruthy() const override { return value != 0.0 && !std::isnan(value); }
};

class JSString : public JSValue {
public:
    std::string value;
    explicit JSString(const std::string& v) : value(v) {}
    JSValueType getType() const override { return JSValueType::STRING; }
    std::string toString() const override { return value; }
    double toNumber() const override;
    bool isTruthy() const override { return !value.empty(); }
};

class JSBoolean : public JSValue {
public:
    bool value;
    explicit JSBoolean(bool v) : value(v) {}
    JSValueType getType() const override { return JSValueType::BOOLEAN; }
    std::string toString() const override { return value ? "true" : "false"; }
    double toNumber() const override { return value ? 1.0 : 0.0; }
    bool isTruthy() const override { return value; }
};

class JSNull : public JSValue {
public:
    JSValueType getType() const override { return JSValueType::NULL_TYPE; }
    std::string toString() const override { return "null"; }
    double toNumber() const override { return 0.0; }
    bool isTruthy() const override { return false; }
};

class JSUndefined : public JSValue {
public:
    JSValueType getType() const override { return JSValueType::UNDEFINED; }
    std::string toString() const override { return "undefined"; }
    double toNumber() const override { return std::numeric_limits<double>::quiet_NaN(); }
    bool isTruthy() const override { return false; }
};

class FunctionDeclaration;
class Environment;

class JSFunction : public JSValue {
public:
    std::shared_ptr<FunctionDeclaration> declaration;
    std::shared_ptr<Environment> closure;
    std::shared_ptr<JSValue> prototypeProperty;

    JSFunction(std::shared_ptr<FunctionDeclaration> decl, std::shared_ptr<Environment> env);

    JSValueType getType() const override { return JSValueType::FUNCTION; }
    std::string toString() const override;
    double toNumber() const override { return std::numeric_limits<double>::quiet_NaN(); }
    bool isTruthy() const override { return true; }
};

class JSObject : public JSValue {
public:
    std::unordered_map<std::string, std::shared_ptr<JSValue>> properties;
    std::shared_ptr<JSObject> prototype;
    JSValueType getType() const override { return JSValueType::OBJECT; }
    std::string toString() const override {
        std::string res = "{ ";
        bool first = true;
        for (const auto& kv : properties) {
            if (!first) res += ", ";
            res += kv.first + ": ";
            if (kv.second->getType() == JSValueType::STRING) {
                res += "'" + kv.second->toString() + "'";
            } else {
                res += kv.second->toString();
            }
            first = false;
        }
        res += " }";
        return res;
    }
    double toNumber() const override { return std::numeric_limits<double>::quiet_NaN(); }
    bool isTruthy() const override { return true; }
};

class JSArray : public JSObject {
public:
    std::vector<std::shared_ptr<JSValue>> elements;
    
    JSArray() {}
    explicit JSArray(const std::vector<std::shared_ptr<JSValue>>& elems) : elements(elems) {}

    JSValueType getType() const override { return JSValueType::ARRAY; }
    std::string toString() const override {
        std::string result = "";
        for (size_t i = 0; i < elements.size(); ++i) {
            if (elements[i]->getType() != JSValueType::UNDEFINED && elements[i]->getType() != JSValueType::NULL_TYPE) {
                result += elements[i]->toString();
            }
            if (i < elements.size() - 1) result += ",";
        }
        return result;
    }
};

#include <functional>

// Typedef for native C++ functions callable from JS
using NativeFunctionType = std::function<std::shared_ptr<JSValue>(const std::vector<std::shared_ptr<JSValue>>&)>;

class JSNativeFunction : public JSValue {
public:
    std::string name;
    NativeFunctionType func;
    JSNativeFunction(const std::string& n, NativeFunctionType f) : name(n), func(f) {}
    JSValueType getType() const override { return JSValueType::NATIVE_FUNCTION; }
    std::string toString() const override { return "function " + name + "() { [native code] }"; }
    double toNumber() const override { return std::numeric_limits<double>::quiet_NaN(); }
    bool isTruthy() const override { return true; }
};

#endif // TYPES_H
