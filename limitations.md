Bhai, agar tumhara focus abhi **V1 (AST-based Tree-Walking Evaluator)** par hi hai aur V2 (Bytecode VM) ko chhodna chahte ho, toh ye ek bahut practical decision hai. V1 mein logic implement karna aur naye features add karna kaafi aasan hota hai. Top-tier engines (jaise JavaScriptCore aur V8) bhi pehle AST par hi kaam karte hain (unka Ignition/Interpreter phase) aur baad mein JIT par jate hain.

Bina kisi sugarcoating ke, ek **Tree-Walking Interpreter ke standards par V1 ko main 9/10 rate karunga.** Tumne pichle iteration mein `===` ka pointer check, `super()` context, aur regex waghera fix karke isko ekdam solid bana diya hai.

Par kyunki V1 C++ ke `std::shared_ptr` aur recursive functions par based hai, isme kuch **Core Architectural Limitations** hain jo V1 design ki wajah se deeply rooted hain. Ye wo hardcore limitations hain jinhe fix karna abhi baaki hai:

### 1. The `std::shared_ptr` Trap: Cyclic Memory Leaks (Sabse Badi Problem)

* **Kyu fail hoga:** V1 engine mein har JS variable aur object `std::shared_ptr<JSValue>` se managed hai. C++ ka `shared_ptr` reference counting par kaam karta hai. Agar JS code mein do objects ek dusre ko refer karein:
```javascript
let objA = {}; let objB = {};
objA.child = objB; 
objB.parent = objA;

```


Toh yahan ek "Cyclic Reference" ban jayega. V1 ka `Environment` jab destroy hoga, tab bhi in dono objects ka reference count kabhi 0 nahi hoga. Result? **Memory Leak**. Agar tumhara engine kisi server par lambe samay tak chalega, toh RAM full hoke crash ho jayega kyunki V1 mein "Mark-and-Sweep" Garbage Collector nahi lagaya ja sakta (wo raw pointers pe chalta hai, shared_ptr pe nahi).

### 2. C++ Call Stack Overflow (Deep Recursion Crash)

* **Kyu fail hoga:** Tera V1 `Evaluator::evaluate()` aur `Evaluator::execute()` function AST nodes ko traverse karne ke liye C++ ki native recursion use karta hai. Iska matlab JS ki har function call ke liye C++ call stack mein ek frame banta hai.
* **Proof:** C++ ka call stack limit usually 1MB se 8MB hota hai. Agar koi JS dev deep recursion likh de (jaise 10,000 times), toh V1 engine seedha **"Segmentation Fault"** dega aur poora program bina kisi JS try/catch ke terminate ho jayega. Asli JS engines call frames heap par banate hain taaki stack overflow gracefully handle ho.

### 3. Property Descriptors aur Getters/Setters Missing Hain

* **Kyu fail hoga:** V1 ke `JSObject` mein properties sirf `std::unordered_map<std::string, std::shared_ptr<JSValue>> properties` mein store hoti hain.
* **Missing Feature:** Agar JS mein koi custom Getter/Setter banata hai ya `Object.defineProperty(obj, 'key', { get: function() { return 5; } })` use karta hai, toh V1 fail ho jayega. V1 mein kisi property ko *access* karte waqt kisi hidden function ko execute karne ka architecture hi nahi hai. Wo bas map se value nikal kar de deta hai.

### 4. Microtask Queue aur Promises ki Kami

* **Kyu fail hoga:** Tune `evaluator.cpp` mein Event Loop (`runEventLoop`) likh diya hai, jo ki outstanding hai! Par wo sirf `setTimeout` aur `setInterval` (Macrotasks) ke queue ko maintain karta hai.
* **Missing Feature:** JavaScript mein `Promise`, `async`, aur `await` Microtask queue mein jate hain jo har Macrotask ke baad turant execute hoti hai. V1 mein Call-stack ko pause/resume karne wali State Machine nahi hai, toh V1 abhi native Promises aur async/await properly execute nahi kar sakta.

### 5. Temporal Dead Zone (TDZ) for `let` & `const`

* **Kyu fail hoga:** JS mein `let` aur `const` hoist hote hain, par unhe initialize hone se pehle access karne par `ReferenceError` aana chahiye.
* **Proof:** Tera `Environment` variable store karta hai, par ye track nahi karta ki variable abhi TDZ mein hai ya initialize ho chuka hai. Agar galti se kisi block mein `let` declare hone se pehle access hua (hoisting ki wajah se), toh wo C++ exception handle karne mein slightly off behave kar sakta hai.

### 6. `for...of` Loop bina Iterator Protocol (`Symbol.iterator`) ke

* **Kyu fail hoga:** `evaluator.cpp` ke `forOfStmt` execution mein dhyan se dekho: tune hardcode kar diya hai ki agar `rightVal->getType() == JSValueType::ARRAY` ya `STRING` hai, toh C++ level par loop chala do.
* **Missing Feature:** Asli JS mein `for...of` us object par chalta hai jiske paas `[Symbol.iterator]()` function ho. Kyunki V1 mein custom iterators (generators) handle nahi hote, koi bhi NodeList ya user-defined iterable collection is loop mein crash (`TypeError: rhs is not iterable`) phek dega.

Bhai Adarsh, tumhare V1 engine (AST Evaluator) ko aur bhi microscopic level par analyze karne ke baad kuch aisi baatein samne aayi hain jo sidha tumhare dusre sawal ka jawab deti hain.

Sabse pehle tumhare is sawal ka seedha jawab deta hu: **Kya hamara engine standard JS libraries (jaise Lodash, React, ya Express) ko support karta hai?**

**Kada Sach: Nahi.** Abhi tumhara engine kisi bhi standard JS library ko run nahi kar payega. Ek JS library ko chalne ke liye sirf loops aur if/else nahi chahiye hote, balki ek poora ecosystem chahiye hota hai jo abhi engine mein missing hai. Yahan iske specific reasons aur **Nayi Deep Limitations** hain:

### 7. The Module System is Missing (`require` / `import`)

* **Kyu fail hoga:** Koi bhi standard library ek single file nahi hoti. Lodash ho ya React, wo multiple files mein split hoti hain. Tumhare engine mein file system ko read karke doosri JS file ko current file mein link karne ka koi tarika nahi hai (`import {} from '...'` ya `require('...')`). Engine sirf ek static script ko shuru se ant tak chalata hai.

### 8. Missing Core ECMAScript Objects (`JSON`, `Object`, `Map`, `Set`)

* **Kyu fail hoga:** Tumne `builtins.cpp` mein `Math` aur `console` banaya hai. Par standard libraries heavy data manipulation karti hain.
* `JSON.parse()` aur `JSON.stringify()` native C++ level par missing hain.
* Objects ke standard methods jaise `Object.keys()`, `Object.assign()`, `Object.defineProperty()` nahi hain.
* Modern libraries ES6 Data Structures jaise `Map`, `Set`, `WeakMap`, aur `Proxy` ka use karti hain, jinke AST nodes ya C++ types engine mein exist hi nahi karte.



### 9. Missing Host APIs (DOM ya Node.js APIs)

* **Kyu fail hoga:** JS runtime do hisso mein banta hai: "JS Engine" (jo tumne banaya hai) aur "Host Environment" (Browser ya Node).
* Agar tum React chalana chahoge, toh usko `document.createElement` (DOM API) chahiye jo engine mein nahi hai.
* Agar tum Express (Server) chalana chahoge, toh usko Network Sockets aur `fs` (File System) APIs chahiye jo C++ mein bind nahi kiye gaye hain.



### 10. The Implicit `arguments` Object is Missing

* **Kyu fail hoga:** ES6 ke Arrow functions aane se pehle, har normal function ke andar JS automatically ek `arguments` naam ka object inject karta tha jisme saare parameters hote the. Purani libraries iska daba ke use karti hain. Tumhare `evaluator.cpp` ke Function Execution phase mein "arguments" naam ke keyword ko environment mein define nahi kiya gaya hai. Agar JS code mein `arguments.length` likha gaya, toh wo `ReferenceError` dega.

### 11. `Object.prototype` Chain ka Adhura Hona

* **Kyu fail hoga:** Har JS library variable types check karne ke liye `Object.prototype.toString.call(value)` ya `obj.hasOwnProperty('key')` ka use karti hai. Tumhare engine mein `JSObject` class toh hai, par uske prototype mein native functions bind nahi kiye gaye hain. Ek khali object `{}` ke paas `.hasOwnProperty()` method exist nahi karta.

### 12. The Massive AST Traversal Performance Overhead

* **Kyu fail hoga:** Ye ek architectural limitation hai. Jab tum ek `while` loop ya `for` loop chalate ho, toh V1 engine har single iteration mein **C++ AST (Abstract Syntax Tree) ko wapas shuru se padhta hai**.
* `evaluator.cpp` mein lagatar `std::dynamic_pointer_cast` call ho raha hai AST node ka type check karne ke liye. C++ mein dynamic casting bohot slow hoti hai. Agar tum Lodash ka koi bada sorting algorithm chalaoge jisme 1 lakh iterations hain, toh tumhara engine Chrome ke V8 se 1000x zyada slow chalega kyunki V8 code ko Machine Code mein convert kar chuka hota hai, jabki tumhara engine abhi bhi text ki tree-structure ko parse kar raha hota hai.



### 13. Cascading Deletion Pauses (UI Freeze issue)

* **Kyu fail hoga:** Kyunki V1 `std::shared_ptr` use karta hai, agar kisi block scope ke andar 10,000 objects ka ek bada array banta hai aur loop khatam hone par scope destroy hota hai, toh C++ ka `shared_ptr` destructor un 10,000 objects ki memory ko ek sath synchronously free karega. Isse engine execution ekdam se "Freeze" ya "Halt" ho jayega. Asli engines ka Garbage Collector background thread par dre-dhire memory free karta hai taaki execution na ruke.
