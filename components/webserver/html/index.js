let hasNowData = false;
let isScanningNow = false;

function connectWebSocket() {
  let ws = new WebSocket("ws://" + location.host + "/ws");

  ws.onopen = () => {
    console.log("WebSocket connected");
  };

  ws.onmessage = (e) => {
    console.log("WS:", e.data);
  };

  ws.onerror = (err) => {
    console.error("WebSocket error:", err);
  };

  ws.onclose = () => {
    console.log("WebSocket closed");
  };
}

// Make sure DOM is ready
window.addEventListener("DOMContentLoaded", () => {
  connectWebSocket();
  setTimeout(() => {
    scan_now();
  }, 1000);
});
