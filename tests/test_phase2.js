// 1. String methods
let str = "  Hello World  ";
let trimmed = str.trim();
if (trimmed == "Hello World" &&
    trimmed.toLowerCase() == "hello world" &&
    trimmed.toUpperCase() == "HELLO WORLD" &&
    trimmed.substring(0, 5) == "Hello" &&
    trimmed.replace("World", "JS") == "Hello JS") {
    console.log("PASS: String methods work");
} else {
    console.log("FAIL: String methods failed");
}

// 2. Array methods
let arr = [1, 2, 3];
arr.shift(); // [2, 3]
arr.unshift(0, 1); // [0, 1, 2, 3]
let sliced = arr.slice(1, 3); // [1, 2]
arr.splice(1, 2, 8, 9); // arr = [0, 8, 9, 3]
if (arr[0] == 0 && arr[1] == 8 && arr[2] == 9 && arr[3] == 3 && sliced[0] == 1 && sliced[1] == 2) {
    console.log("PASS: Array methods work");
} else {
    console.log("FAIL: Array methods failed");
}

// 3. Date
let d = new Date();
let t = d.getTime();
if (t > 1700000000000) {
    console.log("PASS: Date mocking is dynamic");
} else {
    console.log("FAIL: Date mocking failed");
}
