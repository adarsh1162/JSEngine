let obj = { a: 1, b: 2 };
console.log("Object.keys:", Object.keys(obj).join(","));
console.log("Object.values:", Object.values(obj).join(","));

let entries = Object.entries(obj);
let entryStrs = [];
for (let i = 0; i < entries.length; i++) {
    entryStrs.push(entries[i][0] + ":" + entries[i][1]);
}
console.log("Object.entries:", entryStrs.join(","));

let obj2 = { c: 3 };
let assigned = Object.assign(obj2, obj);
console.log("Object.assign:", assigned.a, assigned.b, assigned.c);

let arr = [10, 20, 30];
let fe = [];
arr.forEach(function(val, idx) {
    fe.push(val + idx);
});
console.log("Array.forEach:", fe.join(","));

let idx = arr.findIndex(function(val) { return val === 20; });
console.log("Array.findIndex:", idx);

let promises = [1, 2];
Promise.all(promises).then(function(res) {
    console.log("Promise.all:", res.join(","));
});
