function foo({a, b}) {
    console.log(a, b);
}
try {
    foo({a: 10, b: 20});
} catch(e) {
    console.log("Error:", e.message);
}
