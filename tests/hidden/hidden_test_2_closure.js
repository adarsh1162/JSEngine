// Test: Complex Closures & Scope Retention
function createAccumulator(initial) {
    let total = initial;
    return function(val) {
        total += val;
        return total;
    };
}

let acc = createAccumulator(10);
console.log("Accumulated: " + acc(5)); // Should print 15
console.log("Accumulated: " + acc(20)); // Should print 35

// Testing nested closures
function multiplier(factor) {
    return function(val) {
        return val * factor;
    }
}
let doubler = multiplier(2);
let tripler = multiplier(3);
console.log("Double 4: " + doubler(4)); // 8
console.log("Triple 4: " + tripler(4)); // 12
