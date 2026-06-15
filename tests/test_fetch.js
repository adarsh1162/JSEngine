console.log("=== Testing Fetch API ===");

console.log("Fetching sample data from JSONPlaceholder...");

fetch("https://jsonplaceholder.typicode.com/todos/1")
  .then(res => res.json())
  .then(data => {
    console.log("Data received:");
    console.log(JSON.stringify(data));
    console.log("=== Fetch Test Completed Successfully ===");
  }, err => {
    console.log("Fetch Error:", err);
  });
