console.log("Start");

setTimeout(function() {
    console.log("Timeout 1 executed");
}, 500);

setTimeout(console.log, 200, "Timeout 2 executed directly");

let count = 0;
let id = setInterval(function() {
    count++;
    console.log("Interval count: " + count);
    if (count >= 3) clearInterval(id);
}, 300);

console.log("End");
