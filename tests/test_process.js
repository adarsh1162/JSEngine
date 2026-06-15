console.log("=== Testing Process and Child Process ===");

console.log("1. Testing process.cwd()...");
let cwd = process.cwd();
console.log("Current working directory:", cwd);

console.log("2. Testing process.getenv()...");
let pathEnv = process.getenv("PATH");
console.log("PATH Environment Variable:", pathEnv ? "Exists (length " + pathEnv.length + ")" : "Not Found");

console.log("3. Testing child_process.execSync()...");
let output = child_process.execSync("echo JSEngine");
console.log("Output from shell:");
console.log(output);

console.log("=== Tests Completed Successfully ===");
// process.exit(0); // Optional: if uncommented, engine exits immediately here.
