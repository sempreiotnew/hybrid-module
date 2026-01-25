let pairedDevices = new Set();

async function scan_now() {
  if (isScanningNow) return;
  isScanningNow = true;

  const loader = document.getElementById("nowLoader");
  const ul = document.getElementById("listNow");

  loader.classList.remove("hidden");

  if (!hasNowData) {
    ul.innerHTML = '<li class="empty-state">Scanning devices...</li>';
  }
}

function updateDeviceList(devices) {
  const ul = document.getElementById("listNow");
  ul.innerHTML = ""; // Clear previous entries

  if (devices.length === 0) {
    ul.innerHTML = '<li class="empty-state">No devices found</li>';
    return;
  }

  devices.forEach((device) => {
    ul.innerHTML = "";

    // Separate paired and unpaired devices
    const paired = devices.filter((n) => pairedDevices.has(n.mac));
    const unpaired = devices.filter((n) => !pairedDevices.has(n.mac));

    // Show paired devices first
    if (paired.length > 0) {
      const pairedHeader = document.createElement("div");
      pairedHeader.className = "section-header";
      pairedHeader.innerHTML = `
                <span class="status-badge paired"></span>
                <span>PAIRED DEVICES (${paired.length})</span>
              `;
      ul.appendChild(pairedHeader);

      paired.forEach((n) => {
        const li = document.createElement("li");
        li.className = "device-paired";
        li.innerHTML = `
                  <div class="network-info">
                    <div class="now-mac">${n.mac}</div>
                    <div class="now-last-msg">RSSI: ${n.rssi} dBm • Last: ${n.last_msg}</div>
                  </div>
                  <button class="unpair-btn" data-mac="${n.mac}">REMOVE</button>
                `;

        const btn = li.querySelector(".unpair-btn");
        btn.addEventListener("click", function (e) {
          e.stopPropagation();
          //   unpair(n.mac);
        });

        btn.addEventListener("touchend", function (e) {
          e.preventDefault();
          e.stopPropagation();
          //   unpair(n.mac);
        });

        ul.appendChild(li);
      });
    }

    // Show unpaired devices
    if (unpaired.length > 0) {
      const unpairedHeader = document.createElement("div");
      unpairedHeader.className = "section-header";
      unpairedHeader.innerHTML = `
                <span class="status-badge unpaired"></span>
                <span>AVAILABLE DEVICES (${unpaired.length})</span>
              `;
      ul.appendChild(unpairedHeader);

      unpaired.forEach((n) => {
        const li = document.createElement("li");
        li.className = "device-unpaired";
        li.innerHTML = `
                  <div class="network-info">
                    <div class="now-mac">${n.mac}</div>
                    <div class="now-last-msg">RSSI: ${n.rssi} dBm • Last: ${n.last_msg}</div>
                  </div>
                  <button class="pair-btn" data-mac="${n.mac}">PAREAR</button>
                `;

        const btn = li.querySelector(".pair-btn");
        btn.addEventListener("click", function (e) {
          e.stopPropagation();
          //   pair(n.mac);
        });

        btn.addEventListener("touchend", function (e) {
          e.preventDefault();
          e.stopPropagation();
          //   pair(n.mac);
        });

        ul.appendChild(li);
      });
    }
  });

  // Hide loader if you have one
  const loader = document.getElementById("nowLoader");
  loader.classList.add("hidden");

  hasNowData = true;
  isScanningNow = false;
}
