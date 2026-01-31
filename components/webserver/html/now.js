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

async function pair(mac) {
  try {
    const res = await fetch("/api/pair", {
      method: "POST",
      body: JSON.stringify({ mac: mac }),
    });

    if (res.ok) {
      console.log("Sucessfully paired !");
    }
  } catch (e) {
    console.log(e);
  }
}

async function unpair(mac) {
  try {
    const res = await fetch("/api/pair", {
      method: "DELETE",
      body: JSON.stringify({ mac: mac }),
    });

    if (res.ok) {
      console.log("Sucessfully unpaired !");
    }
  } catch (e) {
    console.log(e);
  }
}

function getAddRemovePairHtml(devices, ul, isPair) {
  // Paired devices
  if (devices.length > 0) {
    const pairedHeader = document.createElement("li");
    pairedHeader.className = "section-header";
    pairedHeader.innerHTML = `
      <span class="status-badge ${isPair ? "paired" : "unpaired"}"></span>
      <span>${isPair ? "PAREADO(s)" : "DISPONÍVEIS"} DEVICES (${devices.length})</span>
    `;
    ul.appendChild(pairedHeader);

    devices.forEach((n) => {
      const li = document.createElement("li");
      li.className = `device-${isPair ? "paired" : "unpaired"}`;
      li.innerHTML = `
        <div class="network-info">
          <div class="now-mac">${n.mac}</div>
          <div class="now-last-msg">RSSI: ${n.rssi} dBm • Last: ${n.last_msg || "-"}</div>
        </div>
        <button class="${isPair ? "unpair" : "pair"}-btn" data-mac="${n.mac}">${isPair ? "REMOVER" : "PAREAR"}</button>
      `;

      const btn = isPair
        ? li.querySelector(".unpair-btn")
        : li.querySelector(".pair-btn");

      btn.addEventListener("click", (e) => {
        e.stopPropagation();
        isPair ? unpair(n.mac) : pair(n.mac);
      });
      btn.addEventListener("touchend", (e) => {
        e.preventDefault();
        e.stopPropagation();
        isPair ? unpair(n.mac) : pair(n.mac);
      });

      ul.appendChild(li);
    });
  }
}

function updateDeviceList(mac, devices) {
  const ul = document.getElementById("listNow");
  const titleInfo = document.getElementById("titleInfo");
  titleInfo.textContent = mac;
  ul.innerHTML = "";

  if (!devices || devices.length === 0) {
    ul.innerHTML = '<li class="empty-state">No devices found</li>';
    return;
  }

  const paired = devices.filter((n) => n.paired);
  const unpaired = devices.filter((n) => !n.paired);

  getAddRemovePairHtml(paired, ul, true);
  getAddRemovePairHtml(unpaired, ul, false);

  // Hide loader
  const loader = document.getElementById("nowLoader");
  loader.classList.add("hidden");

  hasNowData = true;
  isScanningNow = false;
}

function getSignalStrength(rssi) {
  if (rssi >= -30) return { level: 4, class: "excellent" };
  if (rssi >= -50) return { level: 3, class: "good" };
  if (rssi >= -70) return { level: 2, class: "fair" };
  return { level: 1, class: "poor" };
}

function createSignalBars(rssi) {
  const signal = getSignalStrength(rssi);
  let bars = '<span class="signal-strength ' + signal.class + '">';
  for (let i = 1; i <= 4; i++) {
    const opacity = i <= signal.level ? "1" : "0.2";
    bars += '<span class="signal-bar" style="opacity: ' + opacity + '"></span>';
  }
  bars += "</span>";
  return bars;
}

function renderTree(data) {
  const treeDiv = document.getElementById("tree");
  treeDiv.innerHTML = "";

  // Separate paired and unpaired devices
  const pairedDevices = data.payload.filter((d) => d.paired);
  const unpairedDevices = data.payload.filter((d) => !d.paired);

  // ========== CONNECTED SECTION ==========
  if (pairedDevices.length > 0) {
    const connectedSection = document.createElement("div");
    connectedSection.className = "connected-section";

    // Create root node
    const rootNode = document.createElement("div");
    rootNode.className = "node root-node";
    rootNode.innerHTML = `
                    <div class="mac">📡 ${data.mac}</div>
                    <div class="details">Root Device</div>
                `;

    connectedSection.appendChild(rootNode);

    // Main vertical line from root
    const verticalLine = document.createElement("div");
    verticalLine.className = "vertical-line";
    connectedSection.appendChild(verticalLine);

    // Horizontal container for paired devices only
    const horizontalContainer = document.createElement("div");
    horizontalContainer.className = "horizontal-container";

    // Horizontal line connecting paired devices
    if (pairedDevices.length > 1) {
      const deviceWidth = 150; // Reduced for mobile
      const gap = 15; // Reduced gap for mobile
      const totalWidth =
        pairedDevices.length * deviceWidth + (pairedDevices.length - 1) * gap;

      const horizontalLine = document.createElement("div");
      horizontalLine.className = "horizontal-line";
      horizontalLine.style.width = `${totalWidth - 80}px`; // Adjusted for smaller devices
      horizontalLine.style.left = "50%";
      horizontalLine.style.transform = "translateX(-50%)";
      horizontalContainer.appendChild(horizontalLine);
    }

    // Create branches for paired devices only
    pairedDevices.forEach((device) => {
      const branchContainer = document.createElement("div");
      branchContainer.className = "branch-container";

      // Branch line
      const branchLine = document.createElement("div");
      branchLine.className = "branch-line";
      branchContainer.appendChild(branchLine);

      // Leaf node
      const leafNode = document.createElement("div");
      leafNode.className = "node leaf-node paired";

      const signalBars = createSignalBars(device.rssi);

      leafNode.innerHTML = `
                        <div class="mac">${device.mac}</div>
                        <div class="details">
                            <span class="rssi">RSSI: ${device.rssi} dBm</span>
                            ${signalBars}
                        </div>
                        <span class="tree-status-badge paired-badge">✓ Paired</span>
                    `;

      branchContainer.appendChild(leafNode);
      horizontalContainer.appendChild(branchContainer);
    });

    connectedSection.appendChild(horizontalContainer);
    treeDiv.appendChild(connectedSection);
  }

  // ========== DISCONNECTED SECTION ==========
  if (unpairedDevices.length > 0) {
    const disconnectedSection = document.createElement("div");
    disconnectedSection.className = "disconnected-section";

    const sectionTitle = document.createElement("div");
    sectionTitle.className = "section-title";
    sectionTitle.textContent = "⚠️ Nearby Unpaired Devices";
    disconnectedSection.appendChild(sectionTitle);

    const disconnectedDevices = document.createElement("div");
    disconnectedDevices.className = "disconnected-devices";

    // Create unpaired device cards (no connectors!)
    unpairedDevices.forEach((device) => {
      const leafNode = document.createElement("div");
      leafNode.className = "node leaf-node unpaired";

      const signalBars = createSignalBars(device.rssi);

      leafNode.innerHTML = `
                        <div class="mac">${device.mac}</div>
                        <div class="details">
                            <span class="rssi">RSSI: ${device.rssi} dBm</span>
                            ${signalBars}
                        </div>
                        <span class="tree-status-badge unpaired-badge">○ Unpaired</span>
                    `;

      disconnectedDevices.appendChild(leafNode);
    });

    disconnectedSection.appendChild(disconnectedDevices);
    treeDiv.appendChild(disconnectedSection);
  }
}
