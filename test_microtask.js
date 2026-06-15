console.log('1');
setTimeout(function() {
    console.log('4 - macrotask');
    queueMicrotask(function() {
        console.log('5 - microtask inside macrotask');
    });
}, 0);
queueMicrotask(function() {
    console.log('3 - microtask');
});
console.log('2');
