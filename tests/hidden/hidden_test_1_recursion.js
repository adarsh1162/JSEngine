// Test: Deep Recursion & Call Stack Limits
function fib(n) {
    if (n < 2) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

// Calculate the 15th Fibonacci number
// 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610
console.log("Fibonacci(15): " + fib(15));
