let hasNowData = false;
let isScanningNow = false;
let ws;
let connectedDeviceMac = null;

let deviceInfo;
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

    if (!connectedDeviceMac && msg.mac) {
      connectedDeviceMac = msg.mac;
      console.log("Bound UI to device:", connectedDeviceMac);
    }

    // 🔒 Ignore messages not from this ESP
    if (msg.mac !== connectedDeviceMac) {
      console.warn("Discarding foreign device update:", msg.mac);
      return;
    }
    switch (msg.action) {
      case "now_nearby_devices_info":
        if (Array.isArray(msg.payload)) {
          // const fakeDevices = [
          //   {
          //     mac: "FA:KE:00:00:00:01",
          //     name: "Fake 1",
          //     rssi: -40,
          //     parent: msg.mac,
          //     paired: false,
          //   },
          //   {
          //     mac: "FA:KE:00:00:00:02",
          //     name: "Fake 2",
          //     rssi: -42,
          //     parent: msg.mac,
          //     paired: false,
          //   },
          //   {
          //     mac: "FA:KE:00:00:00:03",
          //     name: "Fake 3",
          //     rssi: -45,
          //     parent: msg.mac,
          //     paired: true,
          //   },
          //   {
          //     mac: "FA:KE:00:00:00:04",
          //     name: "Fake 4",
          //     rssi: -48,
          //     parent: msg.mac,
          //     paired: false,
          //   },
          //   {
          //     mac: "FA:KE:00:00:00:05",
          //     name: "Fake 5",
          //     rssi: -50,
          //     parent: msg.mac,
          //     paired: true,
          //   },
          //   {
          //     mac: "FA:KE:00:00:00:06",
          //     name: "Fake 6",
          //     rssi: -52,
          //     parent: msg.mac,
          //     paired: false,
          //   },
          //   {
          //     mac: "FA:KE:00:00:00:07",
          //     name: "Fake 7",
          //     rssi: -55,
          //     parent: msg.mac,
          //     paired: true,
          //   },
          //   {
          //     mac: "FA:KE:00:00:00:08",
          //     name: "Fake 8",
          //     rssi: -58,
          //     parent: msg.mac,
          //     paired: false,
          //   },
          // ];

          // const combinedPayload = msg.payload.concat(
          //   fakeDevices.slice(0, Math.max(0, 10 - msg.payload.length)),
          // );

          // msg.payload = combinedPayload;
          // renderTree(msg);

          updateDeviceInfo({
            mac: msg.mac,
            name: deviceInfo?.name ?? msg.mac,
            parent: deviceInfo?.parent ?? "",
            children: deviceInfo?.children ?? [],
          });
          updateDeviceList(msg.mac, msg.payload);
        } else {
          console.error("Invalid payload for update_devices:", msg.payload);
        }
        break;

      case "device_info":
        console.log(msg);
        deviceInfo = msg;
        updateDeviceInfo(deviceInfo);
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
