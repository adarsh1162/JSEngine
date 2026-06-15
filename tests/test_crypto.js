console.log("--- Native Crypto API Test ---");

let hash = crypto.createHash("sha256");
hash.update("Hello, World!");
let digest = hash.digest("hex");

console.log("SHA-256 of 'Hello, World!':");
console.log(digest);

let expected = "dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f";
if (digest === expected) {
    console.log("✅ SHA-256 matches expected output!");
} else {
    console.log("❌ SHA-256 mismatch!");
}

let hash2 = crypto.createHash("sha1");
hash2.update("JSEngine is awesome!");
let digest2 = hash2.digest("hex");

console.log("SHA-1 of 'JSEngine is awesome!':");
console.log(digest2);
