#ifndef CRYPTO_API_H
#define CRYPTO_API_H

#include "evaluator.h"
#include <memory>

std::shared_ptr<JSObject> createCryptoModule(Evaluator* evaluator);

#endif // CRYPTO_API_H
