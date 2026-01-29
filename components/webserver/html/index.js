let hasNowData = false;
let isScanningNow = false;
let ws;
let connectedDeviceMac = null;

function connectWebSocket() {
  ws = new WebSocket("ws://" + location.host + "/ws");

  ws.onopen = () => {
    console.log("WebSocket connected");
  };

  ws.onmessage = (e) => {
    console.log("WS:", e.data);

    let msg;

    try {
      msg = JSON.parse(e.data);
    } catch (err) {
      console.error("Invalid JSON:", e.data);
      return;
    }

    // Check if "action" exists
    if (!msg.action) {
      console.error("No action field in message:", msg);
      return;
    }

    if (!connectedDeviceMac && msg.source) {
      connectedDeviceMac = msg.source;
      console.log("Bound UI to device:", connectedDeviceMac);
    }

    // 🔒 Ignore messages not from this ESP
    if (msg.source !== connectedDeviceMac) {
      console.warn("Discarding foreign device update:", msg.source);
      return;
    }
    switch (msg.action) {
      case "now_nearby_devices_info":
        if (Array.isArray(msg.payload)) {
          updateDeviceList(msg.source, msg.payload);
        } else {
          console.error("Invalid payload for update_devices:", msg.payload);
        }
        break;

      case "remove_device":
        if (msg.payload && msg.payload.mac) {
          // removeDevice(msg.payload.mac);
        } else {
          console.error("Invalid payload for remove_device:", msg.payload);
        }
        break;

      case "update_config":
        if (msg.payload) {
          // updateConfig(msg.payload);
        } else {
          console.error("Invalid payload for update_config:", msg.payload);
        }
        break;

      default:
        console.warn("Unknown action:", msg.action);
        break;
    }
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
