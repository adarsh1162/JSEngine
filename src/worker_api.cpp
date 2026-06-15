#include "worker_api.h"
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <fstream>
#include "lexer.h"
#include "parser.h"

struct WorkerInstance {
    int id;
    std::thread thread;
    std::queue<std::string> toWorkerQueue;
    std::queue<std::string> toMainQueue;
    std::mutex workerMutex;
    std::mutex mainMutex;
    bool isRunning = true;
};

static int g_nextWorkerId = 1;
static std::unordered_map<int, std::shared_ptr<WorkerInstance>> g_workers;
static std::mutex g_globalMutex;

void runWorkerThread(std::shared_ptr<WorkerInstance> instance, std::string filename) {
    try {
        Evaluator workerEval;
        
        // Define _pollFromMainNative
        workerEval.environment->define("_pollFromMainNative", std::make_shared<JSNativeFunction>("_pollFromMainNative", [instance](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
            if (!instance->isRunning) return std::make_shared<JSString>("_TERMINATE_");
            std::lock_guard<std::mutex> lock(instance->workerMutex);
            if (!instance->toWorkerQueue.empty()) {
                std::string msg = instance->toWorkerQueue.front();
                instance->toWorkerQueue.pop();
                return std::make_shared<JSString>(msg);
            }
            return std::make_shared<JSUndefined>();
        }));
        
        // Define _postToMainNative
        workerEval.environment->define("_postToMainNative", std::make_shared<JSNativeFunction>("_postToMainNative", [instance](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
            if (!args.empty()) {
                std::lock_guard<std::mutex> lock(instance->mainMutex);
                instance->toMainQueue.push(args[0]->toString());
            }
            return std::make_shared<JSUndefined>();
        }));
        
        // Polyfill parentPort and polling
        std::string polyfill = R"(
            const parentPort = {
                onmessage: null,
                postMessage: function(msg) {
                    _postToMainNative(msg);
                }
            };
            const _workerInterval = setInterval(() => {
                let msg = _pollFromMainNative();
                if (msg === "_TERMINATE_") {
                    clearInterval(_workerInterval);
                } else if (msg !== undefined && parentPort.onmessage) {
                    parentPort.onmessage({ data: msg });
                }
            }, 10);
        )";
        
        {
            Lexer lexer(polyfill);
            auto tokens = lexer.tokenize();
            Parser parser(tokens);
            auto ast = parser.parse();
            for (const auto& stmt : ast->body) workerEval.hoist(stmt);
            for (const auto& stmt : ast->body) workerEval.execute(stmt);
        }
        
        // Execute the actual worker script
        std::ifstream file(filename);
        if (file.is_open()) {
            std::string code((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            Lexer lexer(code);
            auto tokens = lexer.tokenize();
            Parser parser(tokens);
            auto ast = parser.parse();
            for (const auto& stmt : ast->body) workerEval.hoist(stmt);
            for (const auto& stmt : ast->body) workerEval.execute(stmt);
        } else {
            std::cerr << "Worker Error: Could not open file " << filename << std::endl;
        }
        
        // Start worker event loop
        workerEval.runEventLoop();
        
    } catch (const std::exception& e) {
        std::cerr << "Worker Thread Error: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Worker Thread Unknown Error" << std::endl;
    }
}

std::shared_ptr<JSObject> createWorkerModule(Evaluator* evaluator) {
    evaluator->environment->define("_createWorkerNative", std::make_shared<JSNativeFunction>("_createWorkerNative", [](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
        if (args.empty()) throw RuntimeError("Worker requires filename");
        std::string filename = args[0]->toString();
        
        auto instance = std::make_shared<WorkerInstance>();
        {
            std::lock_guard<std::mutex> lock(g_globalMutex);
            instance->id = g_nextWorkerId++;
            g_workers[instance->id] = instance;
        }
        
        instance->thread = std::thread(runWorkerThread, instance, filename);
        instance->thread.detach(); // Detach to let it run freely
        
        return std::make_shared<JSNumber>(instance->id);
    }));
    
    evaluator->environment->define("_pollWorkerNative", std::make_shared<JSNativeFunction>("_pollWorkerNative", [](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
        if (args.empty()) return std::make_shared<JSUndefined>();
        int id = args[0]->toNumber();
        
        std::shared_ptr<WorkerInstance> instance;
        {
            std::lock_guard<std::mutex> lock(g_globalMutex);
            if (g_workers.find(id) != g_workers.end()) {
                instance = g_workers[id];
            }
        }
        
        if (instance) {
            std::lock_guard<std::mutex> lock(instance->mainMutex);
            if (!instance->toMainQueue.empty()) {
                std::string msg = instance->toMainQueue.front();
                instance->toMainQueue.pop();
                return std::make_shared<JSString>(msg);
            }
        }
        return std::make_shared<JSUndefined>();
    }));
    
    evaluator->environment->define("_postWorkerNative", std::make_shared<JSNativeFunction>("_postWorkerNative", [](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
        if (args.size() < 2) return std::make_shared<JSUndefined>();
        int id = args[0]->toNumber();
        std::string msg = args[1]->toString();
        
        std::shared_ptr<WorkerInstance> instance;
        {
            std::lock_guard<std::mutex> lock(g_globalMutex);
            if (g_workers.find(id) != g_workers.end()) {
                instance = g_workers[id];
            }
        }
        
        if (instance) {
            std::lock_guard<std::mutex> lock(instance->workerMutex);
            instance->toWorkerQueue.push(msg);
        }
        return std::make_shared<JSUndefined>();
    }));
    
    evaluator->environment->define("_terminateWorkerNative", std::make_shared<JSNativeFunction>("_terminateWorkerNative", [](const std::vector<std::shared_ptr<JSValue>>& args) -> std::shared_ptr<JSValue> {
        if (args.empty()) return std::make_shared<JSUndefined>();
        int id = args[0]->toNumber();
        
        std::shared_ptr<WorkerInstance> instance;
        {
            std::lock_guard<std::mutex> lock(g_globalMutex);
            if (g_workers.find(id) != g_workers.end()) {
                instance = g_workers[id];
                g_workers.erase(id); // Remove from tracking
            }
        }
        
        if (instance) {
            instance->isRunning = false; // signals the background thread to exit its interval
        }
        return std::make_shared<JSUndefined>();
    }));
    
    // Inject the main thread Worker class polyfill
    std::string polyfill = R"(
        class Worker {
            constructor(filename) {
                this._id = _createWorkerNative(filename);
                this.onmessage = null;
                
                let self = this;
                this._interval = setInterval(() => {
                    let msg = _pollWorkerNative(self._id);
                    if (msg !== undefined && self.onmessage) {
                        self.onmessage({ data: msg });
                    }
                }, 10);
            }
            
            postMessage(msg) {
                _postWorkerNative(this._id, msg);
            }
            
            terminate() {
                _terminateWorkerNative(this._id);
                clearInterval(this._interval);
            }
        }
    )";
    
    try {
        Lexer lexer(polyfill);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto ast = parser.parse();
        if (ast) {
            for (const auto& stmt : ast->body) evaluator->hoist(stmt);
            for (const auto& stmt : ast->body) evaluator->execute(stmt);
        }
    } catch (const std::exception& e) {
        std::cerr << "Worker Polyfill Parse Error: " << e.what() << std::endl;
    }
    
    return nullptr; // We injected Worker globally
}
