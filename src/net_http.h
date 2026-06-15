#ifndef NET_HTTP_H
#define NET_HTTP_H

#include "evaluator.h"
#include <memory>

std::shared_ptr<JSObject> createHttpModule(Evaluator* evaluator);

#endif
