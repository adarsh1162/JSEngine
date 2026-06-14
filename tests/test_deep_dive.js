console.log("=== Arrow Function `this` ===");
const obj = {
    value: 42,
    getArrow: function() {
        return () => this.value;
    }
};
const arrow = obj.getArrow();
console.log(arrow() === 42 ? "[PASS] Arrow Function Lexical this" : "[FAIL] Arrow Function Lexical this");

console.log("\n=== Regex .test() and String .match() ===");
const re = /hello/i;
const str = "Hello World";
console.log(re.test(str) ? "[PASS] Regex .test()" : "[FAIL] Regex .test()");
const matches = str.match(re);
console.log(matches && matches[0] === "Hello" ? "[PASS] String .match()" : "[FAIL] String .match()");

console.log("\n=== Object Strict Equality (===) ===");
const a = { x: 1 };
const b = { x: 1 };
const c = a;
console.log(a === c ? "[PASS] a === c" : "[FAIL] a === c");
console.log(a !== b ? "[PASS] a !== b" : "[FAIL] a !== b");

console.log("\n=== Bitwise int32_t Precision ===");
const maxUint32 = 4294967295;
console.log(~0 === -1 ? "[PASS] ~0 === -1" : "[FAIL] ~0 === -1");

console.log("\n=== Super Constructor Calls ===");
class Animal {
    constructor(name) {
        this.name = name;
    }
}
class Dog extends Animal {
    constructor(name, breed) {
        super(name);
        this.breed = breed;
    }
}
const dog = new Dog("Tommy", "Labrador");
console.log(dog.name === "Tommy" && dog.breed === "Labrador" ? "[PASS] Super Constructor" : "[FAIL] Super Constructor");
console.log(re.source);  
