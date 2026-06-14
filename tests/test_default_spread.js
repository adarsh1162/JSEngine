function greet(name = "Guest", greeting = "Hello") {
    console.log(greeting + ", " + name + "!");
}
greet();
greet("Alice");
greet("Bob", "Hi");
greet(undefined, "Welcome");

let nums = [2, 3, 4];
function sum(a, b, c, d) {
    return a + b + c + d;
}
console.log("Sum: " + sum(1, ...nums));

let str = "AB";
console.log("Sum strings: " + sum("X", ...str, "Z"));
