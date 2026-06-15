#ifndef WORKER_API_H
#define WORKER_API_H

#include "evaluator.h"
#include <memory>

std::shared_ptr<JSObject> createWorkerModule(Evaluator* evaluator);

#endif
