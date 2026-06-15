#ifndef NET_WS_H
#define NET_WS_H

#include "evaluator.h"
#include <memory>

std::shared_ptr<JSObject> createWsModule(Evaluator* evaluator);

#endif // NET_WS_H
