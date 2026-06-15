console.log("=== Testing File System (fs) ===");

let testFile = "tests/temp_test.txt";
let testDir = "tests/temp_dir";

// 1. Test writeFileSync
console.log("1. Writing to file...");
fs.writeFileSync(testFile, "Hello, World!");
console.log("File exists?", fs.existsSync(testFile));

// 2. Test appendFileSync
console.log("2. Appending to file...");
fs.appendFileSync(testFile, " JSEngine is awesome!");

// 3. Test readFileSync
console.log("3. Reading from file...");
let content = fs.readFileSync(testFile);
console.log("Content:", content);

// 4. Test mkdirSync
console.log("4. Creating directory...");
fs.mkdirSync(testDir);
console.log("Directory exists?", fs.existsSync(testDir));

// 5. Test unlinkSync
console.log("5. Cleaning up file...");
fs.unlinkSync(testFile);
console.log("File exists after unlink?", fs.existsSync(testFile));

// Note: Removing directories isn't in our simple API yet, so we just leave the temp_dir.

console.log("=== FS Tests Completed Successfully ===");
