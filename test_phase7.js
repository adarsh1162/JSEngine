// Test File for Phase 7: Advanced Native Features & Polyfills

// 1. JSON Support
let obj = { a: 1, b: "hello", c: [1, 2, 3] };
console.log("JSON Stringify Output:");
console.log(JSON.stringify(obj));
console.log("------------------------");

// 2. Map Support
let map = new Map();
map.set("name", "V1 Engine");
map.set("version", 1);
console.log("Map Tests:");
console.log("Map size: " + map.size);
console.log("Map get 'name': " + map.get("name"));
console.log("Map has 'version': " + map.has("version"));
map.delete("name");
console.log("Map has 'name' after delete: " + map.has("name"));
console.log("------------------------");

// 3. Set Support
let set = new Set();
set.add(10);
set.add(10); // Duplicate should be ignored
set.add(20);
console.log("Set Tests:");
console.log("Set size: " + set.size);
console.log("Set has 10: " + set.has(10));
console.log("Set has 30: " + set.has(30));
set.delete(10);
console.log("Set has 10 after delete: " + set.has(10));
console.log("------------------------");

// 4. Object.defineProperty (Getters/Setters)
let person = { _age: 25 };
Object.defineProperty(person, "age", {
    get: function() { return this._age; },
    set: function(val) { if (val >= 0) this._age = val; }
});
console.log("Object.defineProperty Tests:");
console.log("Initial Age: " + person.age);
person.age = 30;
console.log("Updated Age: " + person.age);
person.age = -5; // Should be ignored
console.log("Invalid Update Age: " + person.age);
console.log("------------------------");

// 5. Iterator Protocol (for...of)
console.log("for...of Array Iterator Tests:");
let arr = [100, 200, 300];
for (let val of arr) {
    console.log(val);
}
console.log("------------------------");
