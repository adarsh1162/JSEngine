#include "builtins.h"
#include <algorithm>

std::shared_ptr<JSValue> getArrayMethod(std::shared_ptr<JSArray> array, const std::string& methodName, std::function<std::shared_ptr<JSValue>(std::shared_ptr<JSFunction>, const std::vector<std::shared_ptr<JSValue>>&)> invokeFunc) {
    if (methodName == "filter") {
        return std::make_shared<JSNativeFunction>("filter", [array, invokeFunc](const std::vector<std::shared_ptr<JSValue>>& args) {
            if (args.empty() || args[0]->getType() != JSValueType::FUNCTION || !invokeFunc) return std::shared_ptr<JSValue>(std::make_shared<JSArray>());
            auto callback = std::dynamic_pointer_cast<JSFunction>(args[0]);
            auto resultArr = std::make_shared<JSArray>();
            for (size_t i = 0; i < array->elements.size(); ++i) {
                auto res = invokeFunc(callback, {array->elements[i], std::make_shared<JSNumber>(i), array});
                if (res && res->isTruthy()) resultArr->elements.push_back(array->elements[i]);
            }
            return std::shared_ptr<JSValue>(resultArr);
        });
    }
    if (methodName == "map") {
        return std::make_shared<JSNativeFunction>("map", [array, invokeFunc](const std::vector<std::shared_ptr<JSValue>>& args) {
            if (args.empty() || args[0]->getType() != JSValueType::FUNCTION || !invokeFunc) return std::shared_ptr<JSValue>(std::make_shared<JSArray>());
            auto callback = std::dynamic_pointer_cast<JSFunction>(args[0]);
            auto resultArr = std::make_shared<JSArray>();
            for (size_t i = 0; i < array->elements.size(); ++i) {
                auto res = invokeFunc(callback, {array->elements[i], std::make_shared<JSNumber>(i), array});
                resultArr->elements.push_back(res ? res : std::make_shared<JSUndefined>());
            }
            return std::shared_ptr<JSValue>(resultArr);
        });
    }
    if (methodName == "reduce") {
        return std::make_shared<JSNativeFunction>("reduce", [array, invokeFunc](const std::vector<std::shared_ptr<JSValue>>& args) {
            if (args.empty() || args[0]->getType() != JSValueType::FUNCTION || !invokeFunc) return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
            auto callback = std::dynamic_pointer_cast<JSFunction>(args[0]);
            size_t startIndex = 0;
            std::shared_ptr<JSValue> acc;
            if (args.size() > 1) {
                acc = args[1];
            } else if (!array->elements.empty()) {
                acc = array->elements[0];
                startIndex = 1;
            } else {
                return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
            }
            for (size_t i = startIndex; i < array->elements.size(); ++i) {
                acc = invokeFunc(callback, {acc, array->elements[i], std::make_shared<JSNumber>(i), array});
                if (!acc) acc = std::make_shared<JSUndefined>();
            }
            return acc;
        });
    }
    if (methodName == "find") {
        return std::make_shared<JSNativeFunction>("find", [array, invokeFunc](const std::vector<std::shared_ptr<JSValue>>& args) {
            if (args.empty() || args[0]->getType() != JSValueType::FUNCTION || !invokeFunc) return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
            auto callback = std::dynamic_pointer_cast<JSFunction>(args[0]);
            for (size_t i = 0; i < array->elements.size(); ++i) {
                auto res = invokeFunc(callback, {array->elements[i], std::make_shared<JSNumber>(i), array});
                if (res && res->isTruthy()) return array->elements[i];
            }
            return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
        });
    }
    if (methodName == "some") {
        return std::make_shared<JSNativeFunction>("some", [array, invokeFunc](const std::vector<std::shared_ptr<JSValue>>& args) {
            if (args.empty() || args[0]->getType() != JSValueType::FUNCTION || !invokeFunc) return std::shared_ptr<JSValue>(std::make_shared<JSBoolean>(false));
            auto callback = std::dynamic_pointer_cast<JSFunction>(args[0]);
            for (size_t i = 0; i < array->elements.size(); ++i) {
                auto res = invokeFunc(callback, {array->elements[i], std::make_shared<JSNumber>(i), array});
                if (res && res->isTruthy()) return std::shared_ptr<JSValue>(std::make_shared<JSBoolean>(true));
            }
            return std::shared_ptr<JSValue>(std::make_shared<JSBoolean>(false));
        });
    }
    if (methodName == "every") {
        return std::make_shared<JSNativeFunction>("every", [array, invokeFunc](const std::vector<std::shared_ptr<JSValue>>& args) {
            if (args.empty() || args[0]->getType() != JSValueType::FUNCTION || !invokeFunc) return std::shared_ptr<JSValue>(std::make_shared<JSBoolean>(true));
            auto callback = std::dynamic_pointer_cast<JSFunction>(args[0]);
            for (size_t i = 0; i < array->elements.size(); ++i) {
                auto res = invokeFunc(callback, {array->elements[i], std::make_shared<JSNumber>(i), array});
                if (!res || !res->isTruthy()) return std::shared_ptr<JSValue>(std::make_shared<JSBoolean>(false));
            }
            return std::shared_ptr<JSValue>(std::make_shared<JSBoolean>(true));
        });
    }
    if (methodName == "push") {
        return std::make_shared<JSNativeFunction>("push", [array](const std::vector<std::shared_ptr<JSValue>>& args) {
            for (const auto& arg : args) {
                array->elements.push_back(arg);
            }
            return std::make_shared<JSNumber>(array->elements.size());
        });
    }
    if (methodName == "pop") {
        return std::make_shared<JSNativeFunction>("pop", [array](const std::vector<std::shared_ptr<JSValue>>& args) {
            if (array->elements.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
            auto val = array->elements.back();
            array->elements.pop_back();
            return val;
        });
    }
    if (methodName == "reverse") {
        return std::make_shared<JSNativeFunction>("reverse", [array](const std::vector<std::shared_ptr<JSValue>>& args) {
            std::reverse(array->elements.begin(), array->elements.end());
            return std::shared_ptr<JSValue>(array); // Returns the array itself
        });
    }
    if (methodName == "join") {
        return std::make_shared<JSNativeFunction>("join", [array](const std::vector<std::shared_ptr<JSValue>>& args) {
            std::string sep = ",";
            if (!args.empty() && args[0]->getType() != JSValueType::UNDEFINED) {
                sep = args[0]->toString();
            }
            std::string res;
            for (size_t i = 0; i < array->elements.size(); ++i) {
                res += array->elements[i]->toString();
                if (i < array->elements.size() - 1) res += sep;
            }
            return std::shared_ptr<JSValue>(std::make_shared<JSString>(res));
        });
    }
    if (methodName == "sort") {
        return std::make_shared<JSNativeFunction>("sort", [array](const std::vector<std::shared_ptr<JSValue>>& args) {
            std::sort(array->elements.begin(), array->elements.end(), [](std::shared_ptr<JSValue> a, std::shared_ptr<JSValue> b) {
                // Simplified default sorting: string comparison
                return a->toString() < b->toString();
            });
            return std::shared_ptr<JSValue>(array);
        });
    }
    return std::make_shared<JSUndefined>();
}

std::shared_ptr<JSValue> getStringMethod(std::shared_ptr<JSString> str, const std::string& methodName) {
    if (methodName == "split") {
        return std::make_shared<JSNativeFunction>("split", [str](const std::vector<std::shared_ptr<JSValue>>& args) {
            std::string sep = "";
            if (!args.empty() && args[0]->getType() != JSValueType::UNDEFINED) {
                sep = args[0]->toString();
            }
            auto arr = std::make_shared<JSArray>();
            if (sep.empty()) {
                for (char c : str->value) {
                    arr->elements.push_back(std::make_shared<JSString>(std::string(1, c)));
                }
            } else {
                size_t start = 0;
                size_t end = str->value.find(sep);
                while (end != std::string::npos) {
                    arr->elements.push_back(std::make_shared<JSString>(str->value.substr(start, end - start)));
                    start = end + sep.length();
                    end = str->value.find(sep, start);
                }
                arr->elements.push_back(std::make_shared<JSString>(str->value.substr(start)));
            }
            return std::shared_ptr<JSValue>(arr);
        });
    }
    return std::make_shared<JSUndefined>();
}
