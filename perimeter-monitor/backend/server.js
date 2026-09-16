const express = require('express');
const http = require('http');
const WebSocket = require('ws');

const app = express();
app.use(express.json());

const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

// Keep track of connected dashboard clients
let clients = [];

wss.on('connection', (ws) => {
  console.log('Dashboard connected. Total clients:', clients.length + 1);
  clients.push(ws);

  ws.on('close', () => {
    clients = clients.filter(c => c !== ws);
    console.log('Dashboard disconnected. Total clients:', clients.length);
  });
});

// Broadcast a reading to every connected dashboard
function broadcast(data) {
  const message = JSON.stringify(data);
  clients.forEach(client => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(message);
    }
  });
}

// ESP32 posts its JSON readings here
app.post('/scan-data', (req, res) => {
  const reading = req.body;
  console.log('Received:', reading);
  broadcast(reading);
  res.sendStatus(200);
});

const PORT = 3000;
server.listen(PORT, '0.0.0.0', () => {
  console.log(`Server running on port ${PORT}`);
  console.log(`ESP32 should POST to: http://<this-computer-IP>:${PORT}/scan-data`);
});