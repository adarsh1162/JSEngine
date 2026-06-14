#ifndef BUILTINS_H
#define BUILTINS_H

#include "types.h"
#include <memory>
#include <string>
#include <functional>
#include <vector>

// Retrieve an array method bound to the specific array instance
std::shared_ptr<JSValue> getArrayMethod(std::shared_ptr<JSArray> array, const std::string& methodName, std::function<std::shared_ptr<JSValue>(std::shared_ptr<JSFunction>, const std::vector<std::shared_ptr<JSValue>>&)> invokeFunc = nullptr);

// Retrieve a string method bound to the specific string instance
std::shared_ptr<JSValue> getStringMethod(std::shared_ptr<JSString> str, const std::string& methodName);

#endif // BUILTINS_H
