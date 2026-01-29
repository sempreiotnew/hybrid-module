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
