console.log("--- Native WebSocket Server Test ---");

let server = new ws.WebSocketServer({ port: 8080 });

server.on("connection", (socket) => {
    console.log("Client connected!");
    
    socket.on("message", (msg) => {
        console.log("Received: " + msg);
        socket.send("Echo: " + msg);
    });
    
    socket.on("close", () => {
        console.log("Client disconnected.");
    });
});

console.log("WebSocket Server listening on ws://localhost:8080");

setInterval(() => {
    // Keep alive
}, 1000);
