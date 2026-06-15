// tests/worker_script.js
console.log("[Worker] Started...");

parentPort.onmessage = function(event) {
    console.log("[Worker] Received data ->", event.data);
    
    if (event.data === "Start processing") {
        console.log("[Worker] Processing heavy workload...");
        // Simulate heavy work
        let sum = 0;
        for (let i = 0; i < 500; i++) {
            sum += i;
        }
        console.log("[Worker] Work finished. Sum =", sum);
        parentPort.postMessage("RESULT: " + sum);
        parentPort.postMessage("DONE");
    } else {
        parentPort.postMessage("ACK: " + event.data);
    }
};
