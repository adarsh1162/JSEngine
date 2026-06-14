// ==========================================
// JSEngine FIXED LIMITATIONS TEST SUITE
// ==========================================

let totalTests = 0;
let passedTests = 0;

function assert(condition, testName) {
    totalTests++;
    if (condition === true) {
        passedTests++;
        console.log("✅ [PASS] " + testName);
    } else {
        console.log("❌ [FAIL] " + testName);
    }
}

console.log("Starting Advanced Engine Tests (Checking Fixed Limitations)...\n");

try {
    // ------------------------------------------
    // 1. Bracket Notation (Array & Object)
    // ------------------------------------------
    let arr = [10, 20, 30];
    arr[1] = 99;
    assert(arr[1] === 99 && arr[0] === 10, "Array Bracket Notation Get/Set");

    let obj = { "key": "value" };
    obj["newKey"] = "newValue";
    assert(obj["newKey"] === "newValue", "Object Bracket Notation Get/Set");

    // ------------------------------------------
    // 2. Increment (++), Decrement (--), Break & Continue
    // ------------------------------------------
    let loopSum = 0;
    for (let i = 0; i < 10; i++) {
        if (i === 2) continue; // skip 2
        if (i === 5) break;    // stop at 5
        loopSum += i;          // 0 + 1 + 3 + 4
    }
    assert(loopSum === 8, "++, Break, and Continue Statements");

    // ------------------------------------------
    // 3. Short-Circuiting (&& and ||)
    // ------------------------------------------
    let shortCircuitFlag = false;
    function shouldNotRun() { shortCircuitFlag = true; return true; }

    let result = false && shouldNotRun();
    assert(shortCircuitFlag === false && result === false, "Logical AND (&&) Short-Circuiting");

    let result2 = true || shouldNotRun();
    assert(shortCircuitFlag === false && result2 === true, "Logical OR (||) Short-Circuiting");

    // ------------------------------------------
    // 4. Loose Equality (==) vs Strict (===)
    // ------------------------------------------
    assert((1 == "1") === true, "Loose Equality (1 == '1') Coercion");
    assert((1 === "1") === false, "Strict Equality (1 === '1')");

    // ------------------------------------------
    // 5. 'this' Keyword and OOP Binding
    // ------------------------------------------
    let person = {
        age: 20,
        getAge: function () { return this.age; }
    };
    assert(person.getAge() === 20, "'this' context in Object Methods");

    // ------------------------------------------
    // 6. Template Literals & Ternary Operator
    // ------------------------------------------
    let lang = "JS";
    let str = `Learning ${lang}`;
    assert(str === "Learning JS", "Template Literals with Interpolation");

    let isTrue = (10 > 5) ? "Yes" : "No";
    assert(isTrue === "Yes", "Ternary Operator (? :)");

    // ------------------------------------------
    // 7. Destructuring Assignments
    // ------------------------------------------
    let [x, y] = [5, 10];
    let { name } = { name: "Adarsh", role: "Dev" };
    assert(x === 5 && y === 10 && name === "Adarsh", "Array and Object Destructuring");

    // ------------------------------------------
    // 8. Default Parameters
    // ------------------------------------------
    function greet(msg = "Hello") {
        return msg;
    }
    assert(greet() === "Hello" && greet("Hi") === "Hi", "Function Default Parameters");

    // ------------------------------------------
    // 9. Loop Scoping (Closures with 'let')
    // ------------------------------------------
    let funcs = [];
    for (let j = 0; j < 3; j++) {
        funcs.push(function () { return j; });
    }
    // Agar 'let' block scope fix ho gaya hai, toh 0, 1, 2 aayega. 
    // Agar fix nahi hua toh 'var' ki tarah teeno 3 return karenge.
    assert(funcs[0]() === 0 && funcs[2]() === 2, "Proper Block Scoping in Loops (let)");

    // ------------------------------------------
    // 10. Try...Catch Error Handling & Const Immutability
    // ------------------------------------------
    let caughtError = false;
    try {
        const PI = 3.14;
        PI = 3.15; // Ye line engine ko error throw karwani chahiye
    } catch (e) {
        caughtError = true;
    }
    assert(caughtError === true, "Const Immutability & Try-Catch Block");

    // ------------------------------------------
    // 11. Switch Statement & Do-While
    // ------------------------------------------
    let switchRes = 0;
    let choice = "B";
    switch (choice) {
        case "A": switchRes = 1; break;
        case "B": switchRes = 2; break;
        default: switchRes = 3;
    }
    assert(switchRes === 2, "Switch Statement");

    let doCount = 0;
    do {
        doCount++;
    } while (doCount < 0);
    assert(doCount === 1, "Do-While Loop Execution");

    // ------------------------------------------
    // 12. Extended Built-ins
    // ------------------------------------------
    let testStr = "Adarsh".toLowerCase();
    assert(testStr === "adarsh", "String.toLowerCase()");

    let subStr = "Engine".substring(0, 3);
    assert(subStr === "Eng", "String.substring()");

    let testArr = [1, 2];
    testArr.unshift(0);
    let shifted = testArr.shift();
    assert(shifted === 0 && testArr.length === 2, "Array shift() and unshift()");

} catch (globalError) {
    console.log("❌ [FATAL ERROR] Engine crashed during execution: " + globalError);
}

// ==========================================
// FINAL RESULTS
// ==========================================
console.log("\n------------------------------------");
console.log("Total Advanced Tests: " + totalTests);
console.log("Tests Passed:         " + passedTests);
console.log("------------------------------------");

if (totalTests > 0 && passedTests === totalTests) {
    console.log("🔥 ENGINE STATUS: LEGENDARY! Tumne saari limitations successfully fix kar di hain! Masterpiece!");
} else {
    console.log("⚠️ ENGINE STATUS: PENDING. Kuch limitations abhi bhi properly evaluate nahi ho rahi hain.");
}