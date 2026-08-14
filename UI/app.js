// macOS Suraksha UI Controller
let lockedApps = ["notepad.exe", "calc.exe"];
let settings = {
  protectionEnabled: true,
  useWindowsAuth: true,
  useCustomPin: true,
  autoStartWithWindows: false
};

document.addEventListener("DOMContentLoaded", () => {
  renderAppList();
  updateStatusPill();
});

// Tab Switcher
function switchTab(tabId) {
  document.querySelectorAll(".nav-item").forEach(btn => btn.classList.remove("active"));
  document.querySelectorAll(".tab-pane").forEach(pane => pane.classList.remove("active"));

  event.currentTarget.classList.add("active");
  document.getElementById(`tab-${tabId}`).classList.add("active");
}

// Render Protected Apps List
function renderAppList() {
  const container = document.getElementById("appList");
  const countBadge = document.getElementById("appCountBadge");
  const search = document.getElementById("searchInput").value.toLowerCase();

  container.innerHTML = "";

  const filtered = lockedApps.filter(app => app.toLowerCase().includes(search));
  countBadge.innerText = `${lockedApps.length} Apps`;

  if (filtered.length === 0) {
    container.innerHTML = `<div style="text-align: center; padding: 24px; color: #71717a; font-size: 12px;">No applications protected.</div>`;
    return;
  }

  filtered.forEach(app => {
    const item = document.createElement("div");
    item.className = "mac-app-item";
    item.innerHTML = `
      <div class="mac-app-info">
        <span class="mac-app-title">${escapeHtml(app)}</span>
        <span class="mac-app-sub">${app.includes("\\") ? escapeHtml(app) : "Executable"}</span>
      </div>
      <button class="mac-icon-btn" title="Remove App" onclick="removeApp('${escapeHtml(app)}')">
        <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
          <polyline points="3 6 5 6 21 6"/><path d="M19 6v14a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6m3 0V4a2 2 0 0 1 2-2h4a2 2 0 0 1 2 2v2"/>
        </svg>
      </button>
    `;
    container.appendChild(item);
  });
}

function addPreset(appName) {
  if (!lockedApps.includes(appName)) {
    lockedApps.push(appName);
    renderAppList();
    postToNative({ action: "addApp", app: appName });
  }
}

function removeApp(appName) {
  lockedApps = lockedApps.filter(a => a !== appName);
  renderAppList();
  postToNative({ action: "removeApp", app: appName });
}

function browseAddApp() {
  postToNative({ action: "browseAddApp" });
}

function toggleProtection(enabled) {
  settings.protectionEnabled = enabled;
  updateStatusPill();
  postToNative({ action: "toggleProtection", enabled: enabled });
}

function updateStatusPill() {
  const pill = document.getElementById("macStatusPill");
  const text = document.getElementById("macStatusText");
  if (settings.protectionEnabled) {
    pill.className = "mac-status-pill";
    text.innerText = "Protected";
  } else {
    pill.className = "mac-status-pill paused";
    text.innerText = "Paused";
  }
}

function toggleSetting(key, enabled) {
  settings[key] = enabled;
  postToNative({ action: "updateSetting", key: key, value: enabled });
}

// Passcode Modal Dialog
function openPasscodeModal() {
  document.getElementById("modalNewPin").value = "";
  document.getElementById("modalConfirmPin").value = "";
  document.getElementById("modalError").innerText = "";
  document.getElementById("passcodeModal").classList.add("open");
}

function closePasscodeModal() {
  document.getElementById("passcodeModal").classList.remove("open");
}

function saveNewPasscode() {
  const pin1 = document.getElementById("modalNewPin").value;
  const pin2 = document.getElementById("modalConfirmPin").value;

  if (!pin1) {
    document.getElementById("modalError").innerText = "Passcode cannot be empty.";
    return;
  }
  if (pin1 !== pin2) {
    document.getElementById("modalError").innerText = "Passcodes do not match.";
    return;
  }

  postToNative({ action: "setCustomPinValue", pin: pin1 });
  closePasscodeModal();
}

function lockAllSessions() {
  postToNative({ action: "lockAll" });
}

function postToNative(msg) {
  if (window.chrome && window.chrome.webview) {
    window.chrome.webview.postMessage(JSON.stringify(msg));
  } else {
    console.log("Native Bridge:", msg);
  }
}

if (window.chrome && window.chrome.webview) {
  window.chrome.webview.addEventListener("message", event => {
    try {
      const data = JSON.parse(event.data);
      if (data.type === "init") {
        lockedApps = data.apps || [];
        settings = data.settings || settings;
        document.getElementById("chkProtection").checked = settings.protectionEnabled;
        document.getElementById("chkWinAuth").checked = settings.useWindowsAuth;
        document.getElementById("chkCustomPin").checked = settings.useCustomPin;
        document.getElementById("chkAutoStart").checked = settings.autoStartWithWindows;
        if (data.winUser) {
          document.getElementById("macWinUser").innerText = data.winUser;
        }
        updateStatusPill();
        renderAppList();
      }
    } catch (e) {
      console.error(e);
    }
  });
}

function escapeHtml(str) {
  return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;").replace(/'/g, "&#039;");
}
