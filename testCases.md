Test Cases
Our program will be evaluated against 5 test cases. Each is worth 20 points. Our solution must pass the test case's JavaScript code through our runtime and produce the exact expected output shown below.
1) JS Code (Odd or Even Checker)
let num = 7; 
if (num % 2 === 0) { 
    console.log(num + " is Even"); 
} else { 
    console.log(num + " is Odd"); 
}
Expected Output:
7 is Odd

2) JS Code (Triangle Pattern using For Loop)
for (let i = 1; i <= 5; i++) { 
    let row = ""; 
  
    for (let j = 1; j <= i; j++) { 
        row += "*"; 
    } 
  
    console.log(row); 
}
Ecpected OutPut:
*  
* *  
* * *  
* * * *  
* * * * *
3) JS Code (Armstrong Number Check)
function isArmstrong(num) { let temp = num; let sum = 0; 
while (temp > 0) {
    let digit = temp % 10;
    sum += digit ** 3;
    temp = Math.floor(temp / 10);
}

return sum === num;
  
} 
console.log(isArmstrong(153));
console.log(isArmstrong(123));

Expected Output:true //153 
false // 123

4) JS Code (Array Reverse)
let arr = [1, 2, 3, 4, 5]; 
let reversed = [...arr].reverse(); 
console.log("Original: " + arr.join(", ")); 
console.log("Reversed: " + reversed.join(", "));

Expected Output:
Original: 1, 2, 3, 4, 5 
Reversed: 5, 4, 3, 2, 1

5) JS Code (String Palindrome Check)
let str = "racecar"; 
let reversed = str.split("").reverse().join(""); 
if (str === reversed) { 
    console.log(str + " is a Palindrome"); 
} else { 
    console.log(str + " is not a Palindrome"); 
}

Expected Output:
racecar is a Palindrome


There will be additional hidden test cases used during final evaluation.

These hidden test cases will test similar JavaScript concepts with different inputs, edge cases, and slight variations.

Your runtime should be robust enough to handle any valid JavaScript code covered in class, not just the sample examples provided.