let m = Math.max(1, 2, 3);
let s = Math.sin(0);
let c = Math.cos(0);
let p = Math.pow(2, 3);
let a = Math.abs(-10);
console.log("Math:", m, s, c, p, a, Math.PI > 3);

let d = Date.now();
console.log("Date:", d > 0);

try {
    throw new TypeError("My Type Error");
} catch(e) {
    console.log("Caught:", e.name, "-", e.message);
}

try {
    let uninitialized;
    uninitialized();
} catch (e) {
    console.log("Caught internal:", e.name, "-", e.message);
}
