// do-while test
let a = 0;
do {
    a = a + 1;
} while (a < 5);
console.log("do-while final: " + a);

// switch test
let val = 2;
let res = "";
switch (val) {
    case 1: 
        res = "one";
        break;
    case 2:
        res = "two";
        // fallthrough
    case 3:
        res = res + "three";
        break;
    default:
        res = "unknown";
}
console.log("switch res: " + res);
