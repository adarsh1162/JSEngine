// Test: Spread and Rest Operators
function sumAll(first, ...rest) {
    return rest.reduce(function(acc, val) {
        return acc + val;
    }, first);
}

console.log("Rest Sum: " + sumAll(10, 20, 30, 40)); // 100

let arr1 = [1, 2, 3];
let arr2 = [4, 5, 6];
let merged = [...arr1, 0, ...arr2];

console.log("Spread Merged: " + merged.join(", ")); // 1, 2, 3, 0, 4, 5, 6
