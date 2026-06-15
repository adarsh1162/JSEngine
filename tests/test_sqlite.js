console.log("--- Native SQLite Database Test ---");

// Delete test.db if it exists
if (fs.existsSync("test.db")) {
    fs.unlinkSync("test.db");
}

console.log("Connecting to Database...");
let db = new sqlite.Database("test.db");

console.log("Creating table...");
db.exec("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, age INTEGER)");

console.log("Inserting records...");
db.exec("INSERT INTO users (name, age) VALUES ('Adarsh', 22)");
db.exec("INSERT INTO users (name, age) VALUES ('Alice', 25)");
db.exec("INSERT INTO users (name, age) VALUES ('Bob', 30)");

console.log("Querying records...");
let rows = db.query("SELECT * FROM users");

for (let i = 0; i < rows.length; i++) {
    let row = rows[i];
    console.log("User " + row.id + ": " + row.name + " (" + row.age + ")");
}

console.log("Closing connection...");
db.close();

console.log("✅ SQLite Database functionality working perfectly!");
