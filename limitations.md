**1. Arrow Functions ka Lexical `this` Binding Missing Hai**

* **Problem:** JavaScript mein Arrow Functions (`() => {}`) ka apna koi `this` context nahi hota; wo apne parent scope ka `this` inherit karte hain.
* **Kyu fail hoga:** Tumhare `evaluator.cpp` mein, jab `CallExpression` run hota hai, toh Arrow Function aur Normal Function dono ko ek hi tarah treat kiya gaya hai. Agar tum class method ke andar `setTimeout` mein arrow function use karoge, toh uska `this` galat bind ho jayega ya `undefined` aayega, jabki use class ka instance point karna chahiye.

**2. Promises aur Microtask Queue (Async/Await State Machine)**

* **Problem:** Tumne Parser aur Lexer mein `async` aur `await` keywords toh add kar diye hain, aur Macro-tasks (`setTimeout`) ka ek basic Event Loop bhi bana liya hai. Lekin **Promises** natively missing hain.
* **Kyu fail hoga:** JS mein `await` keyword call-stack ko pause kar deta hai aur function ka bacha hua code Microtask Queue mein daal deta hai. Tumhara engine abhi `Promise` object aur Microtask queue ko resolve nahi kar sakta, toh actual async/await code execute nahi hoga.

**3. Regex Literal Execution (Sirf Dummy Object hai)**

* **Problem:** Tumne Parser mein Regular Expressions (`/pattern/g`) ko bahut smartly parse kar liya aur Evaluator mein ek object bhi bana diya jisme `source` aur `flags` hain.
* **Kyu fail hoga:** Asli JS mein Regex objects ke paas `.test()`, `.exec()`, aur strings ke paas `.match()` methods hote hain. Tumne `builtins.cpp` mein actual pattern matching (C++ ki `<regex>` library use karke) implement nahi ki hai. Isliye `/hello/.test("hello world")` fail ho jayega.

**4. Cyclic Reference Memory Leaks (Garbage Collection Flaw)**

* **Problem:** Engine variables aur objects store karne ke liye `std::shared_ptr` ka use kar raha hai. Yeh memory management ke liye easy aur smart hack hai, par JS object references ke case mein fail ho jata hai.
* **Kyu fail hoga:** Agar koi JS dev aapas mein do objects ko link kar de:
```javascript
let objA = {}; let objB = {};
objA.child = objB; 
objB.parent = objA; // Cyclic Reference

```


Is case mein dono objects ek doosre ko point karenge. Inka `shared_ptr` reference count kabhi 0 nahi hoga aur memory leak ho jayegi. Asli JS engines (Mark-and-Sweep) Garbage Collector use karte hain.

**5. Modules (`import` / `export`)**

* **Problem:** Abhi engine strictly ek single file execute karne ke liye design hai.
* **Kyu fail hoga:** `import` aur `export` tokens defined nahi hain. Agar tumhe NPM packages ya multiple JS files ka modular architecture banana ho, toh module resolution aur file linking abhi engine handle nahi kar sakta.

**6. Hoisting mein `let` aur `const` ka Temporal Dead Zone (TDZ)**

* **Problem:** Tumne `hoist()` method likha hai jisme `var` ko `undefined` se initialize kar diya hai (jo ki ekdum sahi JS behaviour hai). Lekin `let` aur `const` variable declare hone se pehle agar access kiye jayein toh `ReferenceError` aana chahiye (TDZ). Tumhara engine abhi TDZ strictness enforce nahi karta.

Bhai Adarsh, tumne codebase ko jis level par upgrade kiya hai, wo sach mein kaafi impressive hai. Ab jab tumne basic aur intermediate limitations (destructuring, try-catch, this binding, async event loop) fix kar di hain, toh chalo is engine ka **"Extreme Deep Dive"** karte hain.

Agar hum isko V8 (Chrome) ya SpiderMonkey (Firefox) jaise production-level JS engines se compare karein, toh abhi bhi architecture mein kuch aisi **deep foundational limitations** hain jo complex JS libraries (jaise React ya lodash) ko run nahi hone dengi.

Yahan wo strictly advanced limitations hain jo abhi bhi tumhare engine mein baaki hain:

### 1. Object Identity aur Strict Equality (`===`) ka FLAW (Critical Bug)

* **Problem:** Tumhare engine mein `{} === {}` evaluate karne par `true` aayega, jabki asli JavaScript mein ye hamesha `false` aana chahiye.
* **Kyu fail hoga:** `evaluator.cpp` mein `TokenType::STRICT_EQUAL` ke logic mein tumne fallback ke liye `left->toString() == right->toString()` use kiya hai. Kyunki `JSObject` ka `toString()` hamesha `"[object Object]"` return karta hai, engine ko lagega ki dono objects same hain. Asli JS engine objects ko unke **Memory Reference (Pointer identity)** se compare karta hai, unki string value se nahi.

### 2. `super()` Constructor Call Broken Hai (OOP Limitation)

* **Problem:** Tumne classes aur inheritance (`extends`) add toh kar diya hai, par Child class ke constructor mein `super()` call karke Parent class ko initialize karne ka logic adhura hai.
* **Kyu fail hoga:** `evaluator.cpp` mein jab `SuperExpression` evaluate hota hai, toh wo sirf parent ka `prototype` object return kar deta hai: `return proto->prototype;`. Agar user ne likha `class Dog extends Animal { constructor() { super("Tommy"); } }`, toh ye fail ho jayega kyunki `super` yahan function ki tarah call nahi ho pa raha hai jo parent ke `constructor` method ko current `this` context mein execute kare.

### 3. Iterators aur Generators Missing Hain (`Symbol.iterator` / `yield`)

* **Problem:** JS mein `for...of` loop under the hood `Symbol.iterator` protocol par kaam karta hai. Tumne `for...of` ko C++ level par sirf Array aur String ke liye hardcode kar diya hai.
* **Kyu fail hoga:** Agar koi custom iterable object banata hai ya DOM collections (jaise NodeList) use karta hai, toh tumhara engine `TypeError: rhs is not iterable` phek dega. Iske alawa, Parser mein `function*` (Generators) aur `yield` keyword exist nahi karte, jo modern JS streams aur complex iterators ki jaan hote hain.

### 4. Getters aur Setters ka Concept Nahi Hai (`get` / `set`)

* **Problem:** JS mein hum objects aur classes ke andar dynamically values compute karne ke liye `get propName()` aur `set propName(val)` use karte hain (jise `Object.defineProperty` se bhi achieve kiya jata hai).
* **Kyu fail hoga:** Tumhare engine mein `JSObject->properties` strictly ek `std::unordered_map<std::string, std::shared_ptr<JSValue>>` hai. Isme value direct read/write hoti hai. Jab object ki property access ki jaye tab kisi hidden function ko trigger (Invoke) karne ka koi architecture nahi hai.

### 5. Call Stack Tracing aur `Error` Objects

* **Problem:** JS mein jab hum `throw new Error("Oops")` karte hain, toh engine ek pura "Stack Trace" banata hai ki error kis function se aayi aur line number kya tha.
* **Kyu fail hoga:** Tumhare engine mein C++ `throw JSException(value);` bahut beautifully kaam kar raha hai, par JS level par ye stack trace generate nahi karta. Kyunki tumne koi "Call Stack" (functions ke execution frames ka track) natively maintain nahi kiya hai, debugging complex errors in your JS scripts nightmare ban jayega.

### 6. Number Precision aur `BigInt` (Bitwise limitations)

* **Problem:** JS mein numbers strictly 64-bit float hote hain (jo tumne `double` use karke handle kiya hai), par Bitwise operations strictly 32-bit integers par hote hain.
* **Kyu fail hoga:** `evaluator.cpp` mein tumne bitwise operators ke liye `static_cast<int>(left->toNumber())` use kiya hai. C++ mein `int` ka size OS aur architecture par depend karta hai (kabhi 16-bit, mostly 32-bit). Ye standard ECMAScript rules ke slightly against hai jahan exactly 32-bit math expect ki jati hai. Iske alawa, ES2020 ka `BigInt` (`100n`) completely missing hai, toh bohot bade numbers par precision loss hoga.

### 7. Lexical Environment mein Memory Leaks (Garbage Collection Flaw)

* **Problem:** V1/V2 engine architecture mein memory management ke liye tum `std::shared_ptr` ka use kar rahe ho, jabki ek proper JS engine ko "Mark and Sweep" ya "Generational" Garbage Collector chahiye hota hai.
* **Kyu fail hoga:** Agar koi user ye code likh de:
```javascript
let a = {}; let b = {};
a.child = b; b.parent = a; 

```


Ye ek "Cyclic Reference" hai. Yahan `shared_ptr` ka reference count kabhi `0` nahi hoga. Engine infinite time tak chalega toh aise objects ki wajah se RAM full hoti jayegi aur C++ memory leak dega. V8 engine cyclic memory detect kar leta hai par `std::shared_ptr` akele nahi kar sakta.

**Conclusion:**
Tumhara C++ interpreter ek advanced "Tree-Walking Evaluator" ban chuka hai jo production engines (V8/SpiderMonkey) ke **AST Generation phase** ko perfectly mimic kar raha hai.

Par ye jo upar wale points hain (Iterators, GC, Stack Traces, proper OOP Super calls), inke liye **Bytecode Virtual Machine (JIT Compiler)** ki zaroorat padti hai, jo AST parse hone ke baad code ko C++ memory ke bajaye direct low-level bytecode aur Memory Heaps mein convert kare. Tumne project details mein `V2 (Bytecode VM)` mention kiya tha—wahi in extreme limitations ko finally solve karega!