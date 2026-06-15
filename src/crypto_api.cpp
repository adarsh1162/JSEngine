#include "crypto_api.h"
#include <windows.h>
#include <bcrypt.h>
#include <iomanip>
#include <sstream>
#include <vector>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

// We will create a C++ wrapper for BCrypt
class NativeHash {
public:
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    std::vector<BYTE> pbHashObject;
    DWORD cbHashObject = 0;
    DWORD cbHash = 0;
    bool finalized = false;

    NativeHash(const std::wstring& algId) {
        if (!NT_SUCCESS(BCryptOpenAlgorithmProvider(&hAlg, algId.c_str(), NULL, 0))) {
            throw std::runtime_error("Failed to open algorithm provider");
        }
        
        DWORD cbData = 0;
        if (!NT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0))) {
            throw std::runtime_error("Failed to get hash object length");
        }
        
        if (!NT_SUCCESS(BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0))) {
            throw std::runtime_error("Failed to get hash length");
        }
        
        pbHashObject.resize(cbHashObject);
        if (!NT_SUCCESS(BCryptCreateHash(hAlg, &hHash, pbHashObject.data(), cbHashObject, NULL, 0, 0))) {
            throw std::runtime_error("Failed to create hash");
        }
    }

    ~NativeHash() {
        if (hHash) BCryptDestroyHash(hHash);
        if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
    }

    void update(const std::string& data) {
        if (finalized) throw std::runtime_error("Hash already digested");
        if (!NT_SUCCESS(BCryptHashData(hHash, (PBYTE)data.data(), data.size(), 0))) {
            throw std::runtime_error("Failed to hash data");
        }
    }

    std::string digest() {
        if (finalized) throw std::runtime_error("Hash already digested");
        std::vector<BYTE> hash(cbHash);
        if (!NT_SUCCESS(BCryptFinishHash(hHash, hash.data(), cbHash, 0))) {
            throw std::runtime_error("Failed to finish hash");
        }
        finalized = true;
        
        std::stringstream ss;
        for (BYTE b : hash) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        }
        return ss.str();
    }
};

std::shared_ptr<JSObject> createCryptoModule(Evaluator* evaluator) {
    auto cryptoModule = std::make_shared<JSObject>();
    cryptoModule->prototype = evaluator->objectPrototype;
    
    cryptoModule->properties["createHash"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("createHash", [evaluator](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
        if (args.empty()) throw RuntimeError("createHash requires algorithm name");
        std::string alg = args[0]->toString();
        std::wstring algW;
        if (alg == "sha256") algW = BCRYPT_SHA256_ALGORITHM;
        else if (alg == "sha1") algW = BCRYPT_SHA1_ALGORITHM;
        else if (alg == "md5") algW = BCRYPT_MD5_ALGORITHM;
        else throw RuntimeError("Unsupported algorithm");
        
        auto nativeHash = std::make_shared<NativeHash>(algW);
        
        auto hashObj = std::make_shared<JSObject>();
        hashObj->prototype = evaluator->objectPrototype;
        
        hashObj->properties["update"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("update", [nativeHash, hashObj](const std::vector<std::shared_ptr<JSValue>>& updateArgs) -> std::shared_ptr<JSValue> {
            if (updateArgs.empty()) throw RuntimeError("update requires data");
            try {
                nativeHash->update(updateArgs[0]->toString());
            } catch (const std::exception& e) {
                throw RuntimeError(e.what());
            }
            return hashObj; // return this for chaining
        }), nullptr, nullptr, true, true, true};
        
        hashObj->properties["digest"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("digest", [nativeHash](const std::vector<std::shared_ptr<JSValue>>& digestArgs) -> std::shared_ptr<JSValue> {
            try {
                std::string encoding = "hex"; // default
                if (!digestArgs.empty()) encoding = digestArgs[0]->toString();
                
                std::string hashHex = nativeHash->digest();
                return std::make_shared<JSString>(hashHex); // For now only hex supported
            } catch (const std::exception& e) {
                throw RuntimeError(e.what());
            }
        }), nullptr, nullptr, true, true, true};
        
        return hashObj;
    }), nullptr, nullptr, true, true, true};

    return cryptoModule;
}
