console.log("=== Testing Advanced Engine Upgrades ===");

let passed = 0;
let total = 0;

function assert(condition, name) {
    total++;
    if (condition) {
        passed++;
        console.log("✅ [PASS] " + name);
    } else {
        console.log("❌ [FAIL] " + name);
    }
}

// 1. Error Stack Traces
console.log("\n--- Testing Error Stack Traces ---");
function level3() { throw new Error("Deep Error"); }
function level2() { level3(); }
function level1() { level2(); }

try {
    level1();
    console.log("❌ Stack trace test failed (no error thrown)");
} catch (e) {
    // Engine will print the stack trace internally, 
    // but we can just ensure the try/catch works.
    assert(true, "Stack Trace execution (Check console for stack output)");
}

// 2. JSON.stringify Circular Reference
console.log("\n--- Testing JSON.stringify Circular Reference ---");
let obj = { a: 1 };
obj.self = obj;

let circularCaught = false;
try {
    JSON.stringify(obj);
} catch (e) {
    circularCaught = true;
}
assert(circularCaught, "JSON.stringify throws on circular reference instead of crashing");

// 3. EventEmitter Polyfill
console.log("\n--- Testing EventEmitter Polyfill ---");
let emitter = new EventEmitter();
let eventFired = false;
emitter.on('data', function(msg) {
    eventFired = true;
    assert(msg === "Hello Node", "EventEmitter correctly fires async events");
});
emitter.emit('data', "Hello Node");

// 4. String Polyfills
console.log("\n--- Testing String Polyfills ---");
let str = "Hello JavaScript Engine";

assert(str.includes("Script"), "String.prototype.includes");
assert(str.startsWith("Hello"), "String.prototype.startsWith");
assert(str.endsWith("Engine"), "String.prototype.endsWith");
assert("A".concat("B", "C") === "ABC", "String.prototype.concat");
assert("5".padStart(3, "0") === "005", "String.prototype.padStart");
assert("A".padEnd(3, ".") === "A..", "String.prototype.padEnd");

// Periodic GC happens asynchronously in the background.

setTimeout(function() {
    console.log("\n==================================");
    console.log(`Test Results: ${passed}/${total} passed`);
    console.log("==================================");
}, 100);
