### 1. Error Stack Traces (Debugging Superpower)

* **Current State:** Engine error aane par sirf message print karta hai (`RuntimeError: X is not defined`), par ye nahi batata ki error kis function mein aur kahan aayi.
* **The Upgrade:** Tumhare paas `CallStackGuard` already hai. Ek `std::vector<std::string> executionStack` banao. Jab bhi `executeFunction` chale, us function ka naam is vector mein `push_back()` karo aur return hone par `pop_back()` karo. Jab `JSException` ya `RuntimeError` throw ho, toh is vector ko reverse karke print kar do.
* **Result:** Tumhe actual Node.js jaisi error milegi:
```text
RuntimeError: Cannot read property
    at fetchUser()
    at main()

```



### 2. Periodic Garbage Collection (OOM Protection)

* **Current State:** Tumhara `markAndSweep()` logic ekdum brilliant hai (properties clear karke cycle todna), lekin ye sirf `Evaluator::interpret()` ke **end** mein call ho raha hai. Agar koi user JS mein infinite game loop ya server chalata hai, toh GC kabhi chalega hi nahi aur RAM full ho jayegi.
* **The Upgrade:** `runEventLoop()` ke andar, jahan tum timer/microtasks check kar rahe ho, wahan ek tick counter lagao. Har 1000 event loop ticks ke baad, ya jab `timerQueue` khali ho aur engine idle ho, tab `markAndSweep()` ko background/idle time mein call kar do. Isse engine hamesha memory-efficient rahega.

### 3. `JSON.stringify` Circular Reference Crash Fix

* **Current State:** Tumne `builtins.cpp` mein recursive `stringify` function banaya hai. Lekin agar kisi user ne JS mein ye likh diya:
```javascript
let a = {}; a.self = a; 
JSON.stringify(a);

```


Toh tumhara C++ function infinite recursion mein chala jayega aur engine crash ho jayega.
* **The Upgrade:** `stringify` function mein ek `std::unordered_set<JSValue*> visited` pass karo. Jab kisi Object ya Array ko stringify karne lago, toh uska pointer `visited` mein daal do. Agar wo pointer dubara milta hai, toh seedha `throw RuntimeError("TypeError: Converting circular structure to JSON")` phek do.

### 4. `EventEmitter` (The Heart of Node.js)

* **Current State:** Tumhare paas `setTimeout`, `fs`, aur `require` aa gaya hai. Asli Node.js environment banne se tum bas ek step door ho.
* **The Upgrade:** Apne polyfill string (jahan tumne `Promise` likha hai) mein ek choti si `class EventEmitter` add kar do:
```javascript
class EventEmitter {
    constructor() { this.events = {}; }
    on(event, cb) { 
        if(!this.events[event]) this.events[event] = [];
        this.events[event].push(cb);
    }
    emit(event, ...args) {
        if(this.events[event]) {
            this.events[event].forEach(cb => queueMicrotask(() => cb(...args)));
        }
    }
}

```


Is ek choti si JS class ko inject karte hi tumhara engine custom event-driven programming support karne lagega.

### 5. Array/String Methods via JS Polyfills (Reduce C++ Bloat)

* **Current State:** Tumne `filter`, `map`, `reduce`, `slice` sab C++ mein likhe hain (`builtins.cpp`). Ye fast toh hain, par engine ka C++ source code bohot bada aur maintain karna mushkil ho raha hai.
* **The Upgrade:** Jaise tumne `Map` aur `Set` ko JS string se inject kiya, waise hi aage chal kar `Array.prototype.flatMap`, `String.prototype.padStart` jaise methods C++ mein likhne ke bajaye seedhe `polyfillCode` string mein JS mein likh dena. Kyunki tumhara engine ab stable hai, "Self-Hosting" (apne hi JS engine mein JS ke features likhna) best aur sabse standard approach hai.


### missing array methods

Tumne jitne methods banaye hain wo sabhi "Native C++ Wrappers" (`JSNativeFunction`) ki tarah return ho rahe hain, jiska matlab hai inki execution speed bohot zyada fast hogi.

**Kya Missing Hai?**
Agar tum string methods ko ekdum 100% complete karna chahte ho, toh abhi ye methods C++ mein exist **nahi** karte hain:

1. `.indexOf(searchString)`
2. `.includes(searchString)`
3. `.startsWith(searchString)` / `.endsWith(searchString)`
4. `.concat()`
5. `.padStart()` / `.padEnd()`

**Mera Suggestion:** Jaise maine pehle bataya tha, ab engine itna capable ho gaya hai ki tumhe in bache hue methods ko C++ (`builtins.cpp`) mein likhne ki zaroorat nahi hai. Tum inhe apne `polyfillCode` (jo tumne `Map`, `Set`, `Promise` ke liye banaya hai) mein direct JS mein define kar sakte ho:

```javascript
// Example polyfill to add in setupGlobalEnvironment
String.prototype.includes = function(search) {
    return this.indexOf(search) !== -1;
};

```
