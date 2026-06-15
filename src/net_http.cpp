#include "net_http.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#undef TokenType
#undef ERROR
#undef DELETE
#endif
#include <iostream>
#include <string>

std::shared_ptr<JSObject> createHttpModule(Evaluator* evaluator) {
    auto httpObj = std::make_shared<JSObject>();
    httpObj->prototype = evaluator->objectPrototype;
    httpObj->properties["createServer"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("createServer", [evaluator](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
        if (args.empty() || (args[0]->getType() != JSValueType::FUNCTION && args[0]->getType() != JSValueType::NATIVE_FUNCTION)) {
            throw RuntimeError("TypeError: createServer requires a callback function");
        }
        auto callback = args[0];
        
        auto serverObj = std::make_shared<JSObject>();
        serverObj->prototype = evaluator->objectPrototype;
        
        serverObj->properties["listen"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("listen", [evaluator, callback](const std::vector<std::shared_ptr<JSValue>>& listenArgs) -> std::shared_ptr<JSValue> {
            if (listenArgs.empty()) throw RuntimeError("TypeError: listen requires a port");
            int port = listenArgs[0]->toNumber();
            
#ifdef _WIN32
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) throw RuntimeError("WSAStartup failed");
            
            SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (listenSocket == INVALID_SOCKET) {
                WSACleanup();
                throw RuntimeError("Socket creation failed");
            }
            
            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = INADDR_ANY;
            serverAddr.sin_port = htons(port);
            
            if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                closesocket(listenSocket);
                WSACleanup();
                throw RuntimeError("Bind failed");
            }
            
            if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
                closesocket(listenSocket);
                WSACleanup();
                throw RuntimeError("Listen failed");
            }
            
            std::cout << "Server listening on port " << port << "..." << std::endl;
            
            if (listenArgs.size() > 1 && (listenArgs[1]->getType() == JSValueType::FUNCTION || listenArgs[1]->getType() == JSValueType::NATIVE_FUNCTION)) {
                evaluator->executeFunction(listenArgs[1], nullptr, {});
            }
            
            while (true) {
                SOCKET clientSocket = accept(listenSocket, NULL, NULL);
                if (clientSocket == INVALID_SOCKET) continue;
                
                char recvbuf[1024];
                int res = recv(clientSocket, recvbuf, 1024, 0);
                if (res > 0) {
                    recvbuf[res] = '\0';
                    std::string reqData(recvbuf);
                    
                    auto reqObj = std::make_shared<JSObject>();
                    reqObj->prototype = evaluator->objectPrototype;
                    
                    size_t methodEnd = reqData.find(' ');
                    if (methodEnd != std::string::npos) {
                        reqObj->properties["method"] = JSPropertyDescriptor{std::make_shared<JSString>(reqData.substr(0, methodEnd)), nullptr, nullptr, true, true, true};
                        size_t pathEnd = reqData.find(' ', methodEnd + 1);
                        if (pathEnd != std::string::npos) {
                            reqObj->properties["url"] = JSPropertyDescriptor{std::make_shared<JSString>(reqData.substr(methodEnd + 1, pathEnd - methodEnd - 1)), nullptr, nullptr, true, true, true};
                        }
                    }
                    
                    auto resObj = std::make_shared<JSObject>();
                    resObj->prototype = evaluator->objectPrototype;
                    resObj->properties["send"] = JSPropertyDescriptor{std::make_shared<JSNativeFunction>("send", [clientSocket](const std::vector<std::shared_ptr<JSValue>>& endArgs) -> std::shared_ptr<JSValue> {
                        std::string responseData = "";
                        if (!endArgs.empty()) responseData = endArgs[0]->toString();
                        
                        std::string httpResponse = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(responseData.length()) + "\r\nConnection: close\r\n\r\n" + responseData;
                        send(clientSocket, httpResponse.c_str(), httpResponse.length(), 0);
                        closesocket(clientSocket);
                        return std::make_shared<JSUndefined>();
                    }), nullptr, nullptr, true, true, true};
                    
                    try {
                        evaluator->executeFunction(callback, nullptr, {reqObj, resObj});
                    } catch (const std::exception& e) {
                        std::cerr << "Server Error: " << e.what() << std::endl;
                        closesocket(clientSocket);
                    } catch (...) {
                        closesocket(clientSocket);
                    }
                } else {
                    closesocket(clientSocket);
                }
            }
#else
            throw RuntimeError("http module is currently only implemented for Windows in this V1 Engine.");
#endif
            return std::make_shared<JSUndefined>();
        }), nullptr, nullptr, true, true, true};
        
        return std::shared_ptr<JSValue>(serverObj);
    }), nullptr, nullptr, true, true, true};
    
    return httpObj;
}
