#ifndef SQLITE_API_H
#define SQLITE_API_H

#include "evaluator.h"
#include <memory>

std::shared_ptr<JSObject> createSqliteModule(Evaluator* evaluator);

#endif // SQLITE_API_H
