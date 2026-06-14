// Test: Type Coercion Edge Cases
console.log("5" + 2); // Should be "52"
console.log(2 + "5"); // Should be "25"
console.log("5" - 2); // Should be 3 (or NaN if unsupported, but string-number subtraction is standard)

let a = 1;
let b = "3";
console.log(a + 2 + b); // "33"

console.log(null == undefined); // true (loose equality)
console.log(null === undefined); // false (strict equality)

let arr1 = [1, 2];
let arr2 = [3, 4];
console.log(arr1 + arr2); // "1,23,4" in JS

console.log(true + 1); // 2
console.log(false + 1); // 1
