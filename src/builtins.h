#ifndef BUILTINS_H
#define BUILTINS_H

#include "types.h"
#include <memory>
#include <string>

// Retrieve an array method bound to the specific array instance
std::shared_ptr<JSValue> getArrayMethod(std::shared_ptr<JSArray> array, const std::string& methodName);

// Retrieve a string method bound to the specific string instance
std::shared_ptr<JSValue> getStringMethod(std::shared_ptr<JSString> str, const std::string& methodName);

#endif // BUILTINS_H
