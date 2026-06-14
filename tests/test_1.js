// ==========================================
// V1 ENGINE: THE GOD-MODE TEST SUITE (13 Fixes)
// ==========================================

let passed = 0;
let total = 0;

function assert(condition, testName) {
    total++;
    if (condition) {
        passed++;
        console.log("✅ [PASS] " + testName);
    } else {
        console.log("❌ [FAIL] " + testName);
    }
}

function expectError(callback, testName) {
    total++;
    let threwError = false;
    try {
        callback();
    } catch (e) {
        threwError = true;
    }
    if (threwError) {
        passed++;
        console.log("✅ [PASS] " + testName + " (Threw Error Successfully)");
    } else {
        console.log("❌ [FAIL] " + testName + " (Did NOT throw error)");
    }
}

console.log("Starting The Ultimate 13-Limitation Test...\n");

// ---------------------------------------------------------
// 1. TDZ (Temporal Dead Zone) Test
// ---------------------------------------------------------
expectError(function () {
    console.log(tdzVar); // Should throw ReferenceError
    let tdzVar = 10;
}, "1. Temporal Dead Zone (TDZ) for let/const");

// ---------------------------------------------------------
// 2. Implicit 'arguments' Object Test
// ---------------------------------------------------------
try {
    function testArgs() {
        return arguments.length + arguments[1];
    }
    assert(testArgs(10, 20, 30) === 23, "2. Implicit 'arguments' object injection");
} catch (e) { console.log("❌ [FAIL] 2. Arguments object - " + e); }

// ---------------------------------------------------------
// 3. Object.prototype Chain & hasOwnProperty Test
// ---------------------------------------------------------
try {
    let emptyObj = {};
    let isProtoLinked = Object.prototype.hasOwnProperty.call(emptyObj, "test") === false;
    assert(isProtoLinked && emptyObj.toString() === "[object Object]", "3. Global Object.prototype Chain");
} catch (e) { console.log("❌ [FAIL] 3. Object.prototype - " + e); }

// ---------------------------------------------------------
// 4. Getters, Setters & Property Descriptors
// ---------------------------------------------------------
try {
    let person = {};
    let internalValue = 0;
    Object.defineProperty(person, "age", {
        get: function () { return internalValue + 10; },
        set: function (val) { internalValue = val; }
    });
    person.age = 20; // Triggers setter
    assert(person.age === 30, "4. Getters, Setters & Property Descriptors");
} catch (e) { console.log("❌ [FAIL] 4. Property Descriptors - " + e); }

// ---------------------------------------------------------
// 5. Core ECMAScript Objects (JSON, Map, Set)
// ---------------------------------------------------------
try {
    let parsed = JSON.parse('{"status": "ok", "code": 200}');
    let myMap = new Map();
    myMap.set("key1", "value1");
    let mySet = new Set([1, 1, 2, 3]);

    assert(parsed.code === 200 && myMap.get("key1") === "value1" && mySet.size === 3, "5. Core Objects (JSON, Map, Set)");
} catch (e) { console.log("❌ [FAIL] 5. Core ECMAScript Objects - " + e); }

// ---------------------------------------------------------
// 6. Custom Iterators with for...of (Symbol.iterator fallback)
// ---------------------------------------------------------
try {
    let customIterable = {
        // Using string "__iterator__" in case Symbol isn't fully implemented yet
        "__iterator__": function () {
            let i = 0;
            return {
                next: function () {
                    i++;
                    return { value: i * 10, done: i > 3 };
                }
            };
        }
    };
    let iterSum = 0;
    for (let val of customIterable) { iterSum += val; } // 10 + 20 + 30
    assert(iterSum === 60, "6. Custom Iterator Protocol in for...of loop");
} catch (e) { console.log("❌ [FAIL] 6. Custom Iterators - " + e); }

// ---------------------------------------------------------
// 7. C++ Call Stack Overflow Protection (Graceful Fail)
// ---------------------------------------------------------
expectError(function () {
    function deepRecurse(n) {
        if (n === 0) return 0;
        return deepRecurse(n - 1) + 1;
    }
    deepRecurse(20000); // Should throw RangeError, NOT crash C++ with SegFault
}, "7. Deep Recursion Stack Overflow Protection");

// ---------------------------------------------------------
// 8 & 9. Modules & Host APIs (Mock Test)
// ---------------------------------------------------------
try {
    // Agar tumne require aur fs banaya hai toh ye chalega
    let fsType = typeof require === "function" ? typeof require('fs') : "undefined";
    assert(fsType !== "undefined", "8 & 9. Module System (require) and Host APIs");
} catch (e) { console.log("❌ [FAIL] 8 & 9. Modules/APIs - " + e); }

// ---------------------------------------------------------
// 10 & 11. Cyclic Memory Leaks & Cascading Deletion (Stress Test)
// ---------------------------------------------------------
try {
    console.log("   -> Running Memory/Deletion Stress Test...");
    let start = Date.now();
    function createCycle() {
        let a = {}; let b = {};
        a.child = b; b.parent = a;
        // Big array to test cascading deletion freeze
        let hugeArray = new Array(50000);
    }
    for (let i = 0; i < 50; i++) {
        createCycle(); // Should not freeze or leak massively
    }
    let end = Date.now();
    assert((end - start) < 2000, "10 & 11. Cyclic Leaks & Cascading Deletion (Did not freeze)");
} catch (e) { console.log("❌ [FAIL] 10 & 11. Memory Stress Test - " + e); }

// ---------------------------------------------------------
// 12. AST Overhead Performance Check
// ---------------------------------------------------------
try {
    let start = Date.now();
    let counter = 0;
    for (let i = 0; i < 100000; i++) {
        counter++; // Testing dynamic_cast removal speed
    }
    let end = Date.now();
    // Agar dynamic_cast gaya hoga, toh ye <100ms me ho jayega.
    assert((end - start) < 500, "12. AST Traversal Overhead Optimization (Fast loop)");
} catch (e) { console.log("❌ [FAIL] 12. AST Performance - " + e); }

// ---------------------------------------------------------
// 13. Microtasks & Promises (Event Loop Priority)
// Note: This prints asynchronously.
// ---------------------------------------------------------
console.log("\nTesting 13: Microtasks & Promises Order...");
console.log("Expected Output: Start -> End -> Promise 1 -> Promise 2 -> Timeout");
console.log("Actual Output:");

console.log("Start");

setTimeout(function () {
    console.log("Timeout");

    // Final Results
    console.log("\n=================================");
    console.log(`TOTAL TESTS: ${total}`);
    console.log(`PASSED:      ${passed}`);
    console.log("=================================");
    if (passed === total) {
        console.log("🏆 ABSOLUTE LEGEND! ENGINE IS PRODUCTION READY!");
    } else {
        console.log("🔧 KUCH FIXES ABHI BHI PENDING HAIN.");
    }
}, 100);

if (typeof Promise !== "undefined") {
    Promise.resolve().then(function () {
        console.log("Promise 1");
    }).then(function () {
        console.log("Promise 2");
    });
} else {
    console.log("❌ [FAIL] Promise is not defined.");
}

console.log("End");