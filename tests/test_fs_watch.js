console.log("--- Native FS Watcher Test ---");

let dir = process.cwd();
console.log("Watching directory: " + dir);

let testFile = dir + "/fs_watch_test.txt";

fs.watch(dir, (event, filename) => {
    console.log("FS Event detected: " + event + " on file " + filename);
    if (filename === "fs_watch_test.txt") {
        console.log("✅ Correct file change detected!");
        // We will terminate the process after 1 successful detection
        process.exit();
    }
});

// Give the watcher a moment to start, then write to a file
setTimeout(() => {
    console.log("Writing to " + testFile + " to trigger watch event...");
    fs.writeFileSync(testFile, "Hello Watcher!");
}, 100);

// Keep the event loop running
setInterval(() => {}, 1000);
