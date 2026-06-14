let obj = { a: 1, b: 2 };
for (let key in obj) {
    console.log("key: " + key + ", val: " + obj[key]);
}

let arr = [10, 20, 30];
for (let val of arr) {
    console.log("val: " + val);
}

let str = "hi";
for (let char of str) {
    console.log("char: " + char);
}
