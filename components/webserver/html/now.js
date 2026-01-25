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
