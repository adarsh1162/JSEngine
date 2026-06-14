let sum = 0;
for (let i = 0; i < 10; i = i + 1) {
    if (i == 5) continue;
    if (i == 8) break;
    sum = sum + i;
}
console.log("Sum: " + sum);
// Expected: 0+1+2+3+4+6+7 = 23
