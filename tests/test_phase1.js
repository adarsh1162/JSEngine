// 1. Const immutability
try {
    const PI = 3.14;
    PI = 5;
    console.log("FAIL: Const reassignment didn't throw");
} catch (e) {
    console.log("PASS: Const reassignment threw: " + e);
}

// 2. Short Circuiting
let called = false;
function doNotCall() {
    called = true;
    return true;
}
false && doNotCall();
true || doNotCall();
if (!called) {
    console.log("PASS: Short circuiting works");
} else {
    console.log("FAIL: Short circuiting failed");
}

// 3. Loose Equality
if (1 == "1" && "1" == 1 && true == 1 && 0 == false) {
    console.log("PASS: Loose equality coercion works");
} else {
    console.log("FAIL: Loose equality failed");
}

// 4. Loop Closures
let funcs = [];
for (let i = 0; i < 3; i++) {
    funcs.push(function() { return i; });
}
if (funcs[0]() == 0 && funcs[1]() == 1 && funcs[2]() == 2) {
    console.log("PASS: Loop closures capture block scope correctly");
} else {
    console.log("FAIL: Loop closures failed. Got: " + funcs[0]() + ", " + funcs[1]() + ", " + funcs[2]());
}

// 5. Deep printing
let obj = { name: "Adarsh", arr: [1, "two", false] };
console.log("Deep Object Print:");
console.log(obj);
