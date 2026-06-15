// tests/test_worker.js
console.log("Main Thread: Starting...");

let worker = new Worker("tests/worker_script.js");

worker.onmessage = function(event) {
    console.log("Main Thread: Received message from worker ->", event.data);
    
    if (event.data === "DONE") {
        console.log("Main Thread: Terminating worker.");
        worker.terminate();
    }
};

console.log("Main Thread: Sending data to worker...");
worker.postMessage("Hello from Main Thread!");
worker.postMessage("Start processing");

console.log("Main Thread: Doing other stuff while worker runs...");
