// 1. Function Hoisting
let a = foo();
function foo() {
    return "foo";
}
if (a == "foo") {
    console.log("PASS: Function hoisting works");
} else {
    console.log("FAIL: Function hoisting failed");
}

// 2. var Hoisting
function testVar() {
    x = 10;
    if (true) {
        var x;
    }
    return x;
}
if (testVar() == 10) {
    console.log("PASS: var hoisting works");
} else {
    console.log("FAIL: var hoisting failed");
}
