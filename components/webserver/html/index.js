let hasNowData = false;
let isScanningNow = false;
let ws;

function connectWebSocket() {
  ws = new WebSocket("ws://" + location.host + "/ws");

  ws.onopen = () => {
    console.log("WebSocket connected");
  };

  ws.onmessage = (e) => {
    console.log("WS:", e.data);

    // Parse the JSON array
    let devices;
    try {
      devices = JSON.parse(e.data);
    } catch (err) {
      console.error("Invalid JSON:", e.data);
      return;
    }

    updateDeviceList(devices);
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
