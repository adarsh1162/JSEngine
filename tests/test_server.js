console.log("=== Testing Native HTTP Server ===");

let server = http.createServer(function(req, res) {
    console.log("Received " + req.method + " request for " + req.url);
    
    if (req.url === "/") {
        res.send("<h1>Welcome to JSEngine Native Web Server!</h1>");
    } else if (req.url === "/api") {
        res.send(JSON.stringify({
            status: "success",
            message: "This is a native JSON response from C++ winsock2!",
            engine: "JSEngine V1"
        }));
    } else {
        res.send("404 Not Found");
    }
});

console.log("Starting server...");
server.listen(3000, function() {
    console.log("Callback triggered! Server is now running.");
    console.log("Open your browser and visit: http://localhost:3000");
});
