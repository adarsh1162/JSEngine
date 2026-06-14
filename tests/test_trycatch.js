function testTryCatch() {
    try {
        console.log("Inside try block");
        throw "Custom Error!";
        console.log("This should not be printed");
    } catch (e) {
        console.log("Caught error: " + e);
        return 42;
    } finally {
        console.log("Finally block executed!");
    }
}

let result = testTryCatch();
console.log("Result: " + result);
