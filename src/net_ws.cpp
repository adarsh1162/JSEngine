#include "net_ws.h"
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <regex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wincrypt.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

#undef TokenType
#undef ERROR
#undef DELETE
#endif

// --- Event Queue for WebSockets ---
struct WsEvent {
    std::string type;
    std::shared_ptr<JSValue> target;
    std::shared_ptr<JSValue> payload;
};

static std::mutex wsMutex;
static std::queue<WsEvent> wsEventQueue;

// --- Windows Crypto API for SHA-1 and Base64 ---
#ifdef _WIN32
std::string computeWebSocketAcceptKey(const std::string& clientKey) {
    std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string concat = clientKey + magic;
    
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::vector<BYTE> hash(20);
    DWORD hashLen = 20;
    std::string base64Hash = "";

    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
            if (CryptHashData(hHash, (const BYTE*)concat.c_str(), concat.length(), 0)) {
                if (CryptGetHashParam(hHash, HP_HASHVAL, hash.data(), &hashLen, 0)) {
                    DWORD base64Len = 0;
                    CryptBinaryToStringA(hash.data(), hashLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &base64Len);
                    if (base64Len > 0) {
                        std::vector<char> base64Buf(base64Len);
                        CryptBinaryToStringA(hash.data(), hashLen, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, base64Buf.data(), &base64Len);
                        base64Hash = std::string(base64Buf.data(), base64Len);
                    }
                }
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
    return base64Hash;
}
#endif

// --- WebSocket Frame creation and parsing ---
std::vector<uint8_t> createWsFrame(const std::string& text) {
    std::vector<uint8_t> frame;
    frame.push_back(0x81); // FIN + Text opcode
    
    size_t len = text.length();
    if (len <= 125) {
        frame.push_back((uint8_t)len);
    } else if (len <= 65535) {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    for (char c : text) {
        frame.push_back(c);
    }
    return frame;
}

// Client thread function
void handleWebSocketClient(SOCKET clientSocket, std::shared_ptr<JSValue> socketObj) {
#ifdef _WIN32
    std::vector<uint8_t> buffer(4096);
    while (true) {
        int bytesReceived = recv(clientSocket, (char*)buffer.data(), buffer.size(), 0);
        if (bytesReceived <= 0) {
            {
                std::lock_guard<std::mutex> lock(wsMutex);
                wsEventQueue.push({"close", socketObj, nullptr});
            }
            closesocket(clientSocket);
            break;
        }

        // Basic WebSocket unmasking and framing (only handling simple text frames for this V1 demo)
        if (bytesReceived >= 2) {
            uint8_t b1 = buffer[0];
            uint8_t b2 = buffer[1];
            
            bool isFinal = (b1 & 0x80) != 0;
            int opcode = b1 & 0x0F;
            bool masked = (b2 & 0x80) != 0;
            uint64_t payloadLen = b2 & 0x7F;
            
            int offset = 2;
            if (payloadLen == 126) {
                payloadLen = (buffer[2] << 8) | buffer[3];
                offset = 4;
            } else if (payloadLen == 127) {
                payloadLen = 0;
                for (int i = 0; i < 8; i++) {
                    payloadLen = (payloadLen << 8) | buffer[2 + i];
                }
                offset = 10;
            }
            
            if (opcode == 8) { // Close frame
                {
                    std::lock_guard<std::mutex> lock(wsMutex);
                    wsEventQueue.push({"close", socketObj, nullptr});
                }
                closesocket(clientSocket);
                break;
            }
            
            if (opcode == 1) { // Text frame
                std::vector<uint8_t> mask(4);
                if (masked) {
                    for (int i = 0; i < 4; i++) mask[i] = buffer[offset++];
                }
                
                std::string message;
                for (uint64_t i = 0; i < payloadLen; i++) {
                    uint8_t dataByte = buffer[offset++];
                    if (masked) dataByte ^= mask[i % 4];
                    message += (char)dataByte;
                }
                
                {
                    std::lock_guard<std::mutex> lock(wsMutex);
                    wsEventQueue.push({"message", socketObj, std::make_shared<JSString>(message)});
                }
            }
        }
    }
#endif
}

std::shared_ptr<JSObject> createWsModule(Evaluator* evaluator) {
    auto wsModule = std::make_shared<JSObject>();
    wsModule->prototype = evaluator->objectPrototype;
    
    // Polling task for WebSocket Events
    TimerTask wsPollTask;
    wsPollTask.triggerTimeMs = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count() + 50;
    wsPollTask.isInterval = true;
    wsPollTask.intervalMs = 50;
    wsPollTask.callback = std::make_shared<JSNativeFunction>("wsPoll", [evaluator](const std::vector<std::shared_ptr<JSValue>>&) {
        std::vector<WsEvent> eventsToProcess;
        {
            std::lock_guard<std::mutex> lock(wsMutex);
            while (!wsEventQueue.empty()) {
                eventsToProcess.push_back(wsEventQueue.front());
                wsEventQueue.pop();
            }
        }
        
        for (const auto& ev : eventsToProcess) {
            auto targetObj = std::dynamic_pointer_cast<JSObject>(ev.target);
            if (targetObj && targetObj->properties.count("_listeners")) {
                auto listenersObj = std::dynamic_pointer_cast<JSObject>(targetObj->properties["_listeners"].value);
                if (listenersObj && listenersObj->properties.count(ev.type)) {
                    auto callback = listenersObj->properties[ev.type].value;
                    if (callback->getType() == JSValueType::FUNCTION || callback->getType() == JSValueType::NATIVE_FUNCTION) {
                        std::vector<std::shared_ptr<JSValue>> args;
                        if (ev.payload) {
                            args.push_back(ev.payload);
                        }
                        evaluator->executeFunction(callback, targetObj, args);
                    }
                }
            }
        }
        return std::make_shared<JSUndefined>();
    });
    evaluator->timerQueue.push_back(wsPollTask);

    auto serverConstructor = std::make_shared<JSNativeFunction>("WebSocketServer", [evaluator](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
        if (args.empty() || args[0]->getType() != JSValueType::OBJECT) {
            throw RuntimeError("WebSocketServer requires an options object (e.g. {port: 8080})");
        }
        
        auto options = std::dynamic_pointer_cast<JSObject>(args[0]);
        if (!options->properties.count("port")) throw RuntimeError("WebSocketServer options must specify a port");
        
        int port = options->properties["port"].value->toNumber();
        
        auto serverObj = std::make_shared<JSObject>();
        serverObj->prototype = evaluator->objectPrototype;
        
        auto listenersObj = std::make_shared<JSObject>();
        serverObj->properties["_listeners"] = JSPropertyDescriptor{listenersObj, nullptr, nullptr, false, false, false};
        
        serverObj->properties["on"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("on", [listenersObj](const std::vector<std::shared_ptr<JSValue>>& onArgs) -> std::shared_ptr<JSValue> {
            if (onArgs.size() < 2) throw RuntimeError("on requires event name and callback");
            std::string eventName = onArgs[0]->toString();
            listenersObj->properties[eventName] = JSPropertyDescriptor{onArgs[1], nullptr, nullptr, true, true, true};
            return std::make_shared<JSUndefined>();
        }), nullptr, nullptr, true, true, true};
        
        // Start background thread for Server Listen
        std::thread([port, evaluator, serverObj]() {
#ifdef _WIN32
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
            
            SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = INADDR_ANY;
            serverAddr.sin_port = htons(port);
            
            bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
            listen(listenSocket, SOMAXCONN);
            
            while (true) {
                SOCKET clientSocket = accept(listenSocket, NULL, NULL);
                if (clientSocket == INVALID_SOCKET) continue;
                
                char recvbuf[4096];
                int res = recv(clientSocket, recvbuf, 4096, 0);
                if (res > 0) {
                    recvbuf[res] = '\0';
                    std::string req(recvbuf);
                    
                    std::regex keyRegex("Sec-WebSocket-Key: (.*)\r\n");
                    std::smatch match;
                    if (std::regex_search(req, match, keyRegex)) {
                        std::string clientKey = match[1].str();
                        std::string acceptKey = computeWebSocketAcceptKey(clientKey);
                        
                        std::string response = 
                            "HTTP/1.1 101 Switching Protocols\r\n"
                            "Upgrade: websocket\r\n"
                            "Connection: Upgrade\r\n"
                            "Sec-WebSocket-Accept: " + acceptKey + "\r\n\r\n";
                            
                        send(clientSocket, response.c_str(), response.length(), 0);
                        
                        // Create a WebSocket object for JS
                        auto socketObj = std::make_shared<JSObject>();
                        socketObj->prototype = evaluator->objectPrototype;
                        
                        auto sockListenersObj = std::make_shared<JSObject>();
                        socketObj->properties["_listeners"] = JSPropertyDescriptor{sockListenersObj, nullptr, nullptr, false, false, false};
                        
                        socketObj->properties["on"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("on", [sockListenersObj](const std::vector<std::shared_ptr<JSValue>>& onArgs) -> std::shared_ptr<JSValue> {
                            if (onArgs.size() < 2) return std::make_shared<JSUndefined>();
                            std::string eventName = onArgs[0]->toString();
                            sockListenersObj->properties[eventName] = JSPropertyDescriptor{onArgs[1], nullptr, nullptr, true, true, true};
                            return std::make_shared<JSUndefined>();
                        }), nullptr, nullptr, true, true, true};
                        
                        socketObj->properties["send"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("send", [clientSocket](const std::vector<std::shared_ptr<JSValue>>& sendArgs) -> std::shared_ptr<JSValue> {
                            if (sendArgs.empty()) return std::make_shared<JSUndefined>();
                            std::string msg = sendArgs[0]->toString();
                            std::vector<uint8_t> frame = createWsFrame(msg);
                            send(clientSocket, (const char*)frame.data(), frame.size(), 0);
                            return std::make_shared<JSUndefined>();
                        }), nullptr, nullptr, true, true, true};
                        
                        // Push connection event
                        {
                            std::lock_guard<std::mutex> lock(wsMutex);
                            wsEventQueue.push({"connection", serverObj, socketObj});
                        }
                        
                        // Spawn client thread to read messages
                        std::thread(handleWebSocketClient, clientSocket, socketObj).detach();
                    }
                }
            }
#endif
        }).detach();
        
        return serverObj;
    });
    
    wsModule->properties["WebSocketServer"] = JSPropertyDescriptor{serverConstructor, nullptr, nullptr, true, true, true};
    
    return wsModule;
}
