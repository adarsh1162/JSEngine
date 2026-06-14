// Test: Math and Date Objects
let rand = Math.random();
console.log("Random number generated: " + (rand >= 0 && rand < 1)); // Should be true

console.log("Max value: " + Math.max(10, 50, 30, 90, 20)); // 90
console.log("Min value: " + Math.min(10, 50, -5, 90, 20)); // -5
console.log("Power: " + Math.pow(2, 4)); // 16
console.log("Round: " + Math.round(4.6)); // 5

let now = new Date();
console.log("Is Date valid object? " + (typeof now === 'object'));
// Assuming basic Date methods are implemented
// Since standard JS Date string format varies, we check if getTime exists
console.log("Has getTime method? " + (typeof now.getTime === 'function'));
