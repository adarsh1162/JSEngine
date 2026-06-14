// Test: Array Callbacks & Method Chaining
let arr = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

let result = arr
    .filter(x => x % 2 !== 0) // [1, 3, 5, 7, 9]
    .map(x => x * x)          // [1, 9, 25, 49, 81]
    .reduce((acc, curr) => acc + curr, 0); // 165

console.log("Complex Array Operation Result: " + result);

let found = arr.find(x => x > 5);
console.log("Find > 5: " + found); // 6

let hasEven = arr.some(x => x % 2 === 0);
console.log("Has even: " + hasEven); // true

let allPositive = arr.every(x => x > 0);
console.log("All positive: " + allPositive); // true
