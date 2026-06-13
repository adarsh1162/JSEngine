#include "builtins.h"
#include <algorithm>

std::shared_ptr<JSValue> getArrayMethod(std::shared_ptr<JSArray> array, const std::string& methodName) {
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
