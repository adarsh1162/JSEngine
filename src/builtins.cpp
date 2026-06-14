#include "builtins.h"
#include <algorithm>
#include <regex>

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
    if (methodName == "shift") {
        return std::make_shared<JSNativeFunction>("shift", [array](const std::vector<std::shared_ptr<JSValue>>& args) {
            if (array->elements.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
            auto val = array->elements.front();
            array->elements.erase(array->elements.begin());
            return val;
        });
    }
    if (methodName == "unshift") {
        return std::make_shared<JSNativeFunction>("unshift", [array](const std::vector<std::shared_ptr<JSValue>>& args) {
            array->elements.insert(array->elements.begin(), args.begin(), args.end());
            return std::shared_ptr<JSValue>(std::make_shared<JSNumber>(array->elements.size()));
        });
    }
    if (methodName == "slice") {
        return std::make_shared<JSNativeFunction>("slice", [array](const std::vector<std::shared_ptr<JSValue>>& args) {
            int start = 0;
            int end = array->elements.size();
            if (args.size() > 0) {
                start = args[0]->toNumber();
                if (start < 0) start = std::max(0, (int)array->elements.size() + start);
            }
            if (args.size() > 1) {
                end = args[1]->toNumber();
                if (end < 0) end = std::max(0, (int)array->elements.size() + end);
            }
            start = std::min((int)array->elements.size(), start);
            end = std::min((int)array->elements.size(), end);
            
            auto resArr = std::make_shared<JSArray>();
            if (start < end) {
                resArr->elements.assign(array->elements.begin() + start, array->elements.begin() + end);
            }
            return std::shared_ptr<JSValue>(resArr);
        });
    }
    if (methodName == "splice") {
        return std::make_shared<JSNativeFunction>("splice", [array](const std::vector<std::shared_ptr<JSValue>>& args) {
            if (args.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSArray>());
            int start = args[0]->toNumber();
            if (start < 0) start = std::max(0, (int)array->elements.size() + start);
            start = std::min((int)array->elements.size(), start);

            int deleteCount = array->elements.size() - start;
            if (args.size() > 1) {
                deleteCount = std::max(0, (int)args[1]->toNumber());
                deleteCount = std::min((int)array->elements.size() - start, deleteCount);
            }

            auto resArr = std::make_shared<JSArray>();
            resArr->elements.assign(array->elements.begin() + start, array->elements.begin() + start + deleteCount);

            array->elements.erase(array->elements.begin() + start, array->elements.begin() + start + deleteCount);
            if (args.size() > 2) {
                array->elements.insert(array->elements.begin() + start, args.begin() + 2, args.end());
            }
            return std::shared_ptr<JSValue>(resArr);
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
    if (methodName == "substring") {
        return std::make_shared<JSNativeFunction>("substring", [str](const std::vector<std::shared_ptr<JSValue>>& args) {
            int start = 0;
            int end = str->value.length();
            if (args.size() > 0) start = std::max(0, (int)args[0]->toNumber());
            if (args.size() > 1) end = std::max(0, (int)args[1]->toNumber());
            start = std::min((int)str->value.length(), start);
            end = std::min((int)str->value.length(), end);
            if (start > end) std::swap(start, end);
            return std::shared_ptr<JSValue>(std::make_shared<JSString>(str->value.substr(start, end - start)));
        });
    }
    if (methodName == "toLowerCase") {
        return std::make_shared<JSNativeFunction>("toLowerCase", [str](const std::vector<std::shared_ptr<JSValue>>& args) {
            std::string res = str->value;
            std::transform(res.begin(), res.end(), res.begin(), ::tolower);
            return std::shared_ptr<JSValue>(std::make_shared<JSString>(res));
        });
    }
    if (methodName == "toUpperCase") {
        return std::make_shared<JSNativeFunction>("toUpperCase", [str](const std::vector<std::shared_ptr<JSValue>>& args) {
            std::string res = str->value;
            std::transform(res.begin(), res.end(), res.begin(), ::toupper);
            return std::shared_ptr<JSValue>(std::make_shared<JSString>(res));
        });
    }
    if (methodName == "trim") {
        return std::make_shared<JSNativeFunction>("trim", [str](const std::vector<std::shared_ptr<JSValue>>& args) {
            std::string res = str->value;
            res.erase(res.begin(), std::find_if(res.begin(), res.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            res.erase(std::find_if(res.rbegin(), res.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), res.end());
            return std::shared_ptr<JSValue>(std::make_shared<JSString>(res));
        });
    }
    if (methodName == "replace") {
        return std::make_shared<JSNativeFunction>("replace", [str](const std::vector<std::shared_ptr<JSValue>>& args) {
            if (args.size() < 2) return std::shared_ptr<JSValue>(str);
            std::string search = args[0]->toString();
            std::string replace = args[1]->toString();
            std::string res = str->value;
            size_t pos = res.find(search);
            if (pos != std::string::npos) {
                res.replace(pos, search.length(), replace);
            }
            return std::shared_ptr<JSValue>(std::make_shared<JSString>(res));
        });
    }
    if (methodName == "match") {
        return std::make_shared<JSNativeFunction>("match", [str](const std::vector<std::shared_ptr<JSValue>>& args) {
            if (args.empty() || args[0]->getType() != JSValueType::OBJECT) return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
            auto regexObj = std::dynamic_pointer_cast<JSObject>(args[0]);
            if (!regexObj->properties.count("source")) return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
            
            std::string pattern = regexObj->properties["source"]->toString();
            if (pattern.length() >= 2 && pattern.front() == '/' && pattern.back() == '/') {
                pattern = pattern.substr(1, pattern.length() - 2);
            }
            std::string flags = regexObj->properties.count("flags") ? regexObj->properties["flags"]->toString() : "";
            
            try {
                std::regex_constants::syntax_option_type opt = std::regex_constants::ECMAScript;
                if (flags.find('i') != std::string::npos) opt |= std::regex_constants::icase;
                std::regex r(pattern, opt);
                
                if (flags.find('g') != std::string::npos) {
                    auto arr = std::make_shared<JSArray>();
                    auto words_begin = std::sregex_iterator(str->value.begin(), str->value.end(), r);
                    auto words_end = std::sregex_iterator();
                    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
                        arr->elements.push_back(std::make_shared<JSString>(i->str()));
                    }
                    if (arr->elements.empty()) return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
                    return std::shared_ptr<JSValue>(arr);
                } else {
                    std::smatch match;
                    if (std::regex_search(str->value, match, r)) {
                        auto arr = std::make_shared<JSArray>();
                        for (size_t i = 0; i < match.size(); ++i) {
                            arr->elements.push_back(std::make_shared<JSString>(match.str(i)));
                        }
                        return std::shared_ptr<JSValue>(arr);
                    }
                    return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
                }
            } catch (...) {
                return std::shared_ptr<JSValue>(std::make_shared<JSUndefined>());
            }
        });
    }
    return std::make_shared<JSUndefined>();
}
