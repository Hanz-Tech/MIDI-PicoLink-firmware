import { serialHandler } from "./serial-handler";
import { validateConfig } from "./validator";

// Helper: log to logbox
function logSent(msg: string) {
  const logbox = document.getElementById("logbox") as HTMLPreElement;
  logbox.textContent += "[SENT] " + msg + "\n";
  logbox.scrollTop = logbox.scrollHeight;
}
function logRecv(msg: string) {
  const logbox = document.getElementById("logbox") as HTMLPreElement;
  logbox.textContent += "[RECV] " + msg + "\n";
  logbox.scrollTop = logbox.scrollHeight;
}
function logInfo(msg: string) {
  const logbox = document.getElementById("logbox") as HTMLPreElement;
  logbox.textContent += "[INFO] " + msg + "\n";
  logbox.scrollTop = logbox.scrollHeight;
}
function logError(msg: string) {
  const logbox = document.getElementById("logbox") as HTMLPreElement;
  logbox.textContent += "[ERROR] " + msg + "\n";
  logbox.scrollTop = logbox.scrollHeight;
}

// Build config JSON from UI
function buildConfigJson() {
  const filters: boolean[][] = [];
  for (let iface = 0; iface < 3; iface++) {
    const arr: boolean[] = [];
    for (let msg = 0; msg < 8; msg++) {
      // REVERSED LOGIC: checked = allowed = true, so store !checked for blocked
      arr.push(!(document.getElementById(`f-${iface}-${msg}`) as HTMLInputElement).checked);
    }
    filters.push(arr);
  }
  const channels: boolean[] = [];
  for (let i = 0; i < 16; i++) {
    channels.push((document.getElementById(`ch-${i}`) as HTMLInputElement).checked);
  }

  // Build IMU configuration
  const rollEnabled = (document.getElementById('roll-enabled') as HTMLInputElement).checked;
  const pitchEnabled = (document.getElementById('pitch-enabled') as HTMLInputElement).checked;
  const yawEnabled = (document.getElementById('yaw-enabled') as HTMLInputElement).checked;
  
  const imu = {
    enabled: rollEnabled || pitchEnabled || yawEnabled, // Auto-enable if any axis is enabled
    roll: {
      enabled: rollEnabled,
      channel: parseInt((document.getElementById('roll-channel') as HTMLInputElement).value),
      cc: parseInt((document.getElementById('roll-cc') as HTMLInputElement).value),
      defaultValue: parseInt((document.getElementById('roll-default') as HTMLInputElement).value),
      toSerial: (document.getElementById('roll-serial') as HTMLInputElement).checked,
      toUSBDevice: (document.getElementById('roll-usb-device') as HTMLInputElement).checked,
      toUSBHost: (document.getElementById('roll-usb-host') as HTMLInputElement).checked,
      sensitivity: parseFloat((document.getElementById('roll-sensitivity') as HTMLInputElement).value),
      range: parseFloat((document.getElementById('roll-range') as HTMLInputElement).value)
    },
    pitch: {
      enabled: pitchEnabled,
      channel: parseInt((document.getElementById('pitch-channel') as HTMLInputElement).value),
      cc: parseInt((document.getElementById('pitch-cc') as HTMLInputElement).value),
      defaultValue: parseInt((document.getElementById('pitch-default') as HTMLInputElement).value),
      toSerial: (document.getElementById('pitch-serial') as HTMLInputElement).checked,
      toUSBDevice: (document.getElementById('pitch-usb-device') as HTMLInputElement).checked,
      toUSBHost: (document.getElementById('pitch-usb-host') as HTMLInputElement).checked,
      sensitivity: parseFloat((document.getElementById('pitch-sensitivity') as HTMLInputElement).value),
      range: parseFloat((document.getElementById('pitch-range') as HTMLInputElement).value)
    },
    yaw: {
      enabled: yawEnabled,
      channel: parseInt((document.getElementById('yaw-channel') as HTMLInputElement).value),
      cc: parseInt((document.getElementById('yaw-cc') as HTMLInputElement).value),
      defaultValue: parseInt((document.getElementById('yaw-default') as HTMLInputElement).value),
      toSerial: (document.getElementById('yaw-serial') as HTMLInputElement).checked,
      toUSBDevice: (document.getElementById('yaw-usb-device') as HTMLInputElement).checked,
      toUSBHost: (document.getElementById('yaw-usb-host') as HTMLInputElement).checked,
      sensitivity: parseFloat((document.getElementById('yaw-sensitivity') as HTMLInputElement).value),
      range: parseFloat((document.getElementById('yaw-range') as HTMLInputElement).value)
    }
  };

  return {
    command: "SAVEALL",
    filters,
    channels,
    imu
  };
}

// Apply config JSON to UI
function applyConfigToUI(config: any, skipValidation = false) {
  const statusDiv = document.getElementById("status") as HTMLDivElement;
  if (!skipValidation && !validateConfig(config)) {
    logError("Invalid config: " + JSON.stringify(validateConfig.errors));
    statusDiv.textContent = "Invalid config: " + JSON.stringify(validateConfig.errors);
    statusDiv.className = "status error";
    return false;
  }
  // Cast config to expected type
  const cfg = config as { filters: boolean[][], channels: boolean[], imu?: any };
  
  // Set filters
  for (let iface = 0; iface < 3; iface++) {
    for (let msg = 0; msg < 8; msg++) {
      // REVERSED LOGIC: checked = allowed = true, so checked = !blocked
      (document.getElementById(`f-${iface}-${msg}`) as HTMLInputElement).checked = !cfg.filters[iface][msg];
    }
  }
  
  // Set channels
  for (let i = 0; i < 16; i++) {
    (document.getElementById(`ch-${i}`) as HTMLInputElement).checked = !!cfg.channels[i];
  }

  // Set IMU configuration if present
  if (cfg.imu) {
    // Note: No global IMU enable checkbox anymore - it's automatically determined by individual axis enables
    
    // Roll configuration
    if (cfg.imu.roll) {
      (document.getElementById('roll-enabled') as HTMLInputElement).checked = !!cfg.imu.roll.enabled;
      (document.getElementById('roll-channel') as HTMLInputElement).value = (cfg.imu.roll.channel || 1).toString();
      (document.getElementById('roll-cc') as HTMLInputElement).value = (cfg.imu.roll.cc || 1).toString();
      (document.getElementById('roll-default') as HTMLInputElement).value = (cfg.imu.roll.defaultValue || 64).toString();
      (document.getElementById('roll-serial') as HTMLInputElement).checked = cfg.imu.roll.toSerial !== false;
      (document.getElementById('roll-usb-device') as HTMLInputElement).checked = cfg.imu.roll.toUSBDevice !== false;
      (document.getElementById('roll-usb-host') as HTMLInputElement).checked = cfg.imu.roll.toUSBHost !== false;
      (document.getElementById('roll-sensitivity') as HTMLInputElement).value = (cfg.imu.roll.sensitivity || 1.0).toString();
      (document.getElementById('roll-range') as HTMLInputElement).value = (cfg.imu.roll.range || 45).toString();
    }
    
    // Pitch configuration
    if (cfg.imu.pitch) {
      (document.getElementById('pitch-enabled') as HTMLInputElement).checked = !!cfg.imu.pitch.enabled;
      (document.getElementById('pitch-channel') as HTMLInputElement).value = (cfg.imu.pitch.channel || 1).toString();
      (document.getElementById('pitch-cc') as HTMLInputElement).value = (cfg.imu.pitch.cc || 2).toString();
      (document.getElementById('pitch-default') as HTMLInputElement).value = (cfg.imu.pitch.defaultValue || 64).toString();
      (document.getElementById('pitch-serial') as HTMLInputElement).checked = cfg.imu.pitch.toSerial !== false;
      (document.getElementById('pitch-usb-device') as HTMLInputElement).checked = cfg.imu.pitch.toUSBDevice !== false;
      (document.getElementById('pitch-usb-host') as HTMLInputElement).checked = cfg.imu.pitch.toUSBHost !== false;
      (document.getElementById('pitch-sensitivity') as HTMLInputElement).value = (cfg.imu.pitch.sensitivity || 1.0).toString();
      (document.getElementById('pitch-range') as HTMLInputElement).value = (cfg.imu.pitch.range || 45).toString();
    }
    
    // Yaw configuration
    if (cfg.imu.yaw) {
      (document.getElementById('yaw-enabled') as HTMLInputElement).checked = !!cfg.imu.yaw.enabled;
      (document.getElementById('yaw-channel') as HTMLInputElement).value = (cfg.imu.yaw.channel || 1).toString();
      (document.getElementById('yaw-cc') as HTMLInputElement).value = (cfg.imu.yaw.cc || 3).toString();
      (document.getElementById('yaw-default') as HTMLInputElement).value = (cfg.imu.yaw.defaultValue || 64).toString();
      (document.getElementById('yaw-serial') as HTMLInputElement).checked = cfg.imu.yaw.toSerial !== false;
      (document.getElementById('yaw-usb-device') as HTMLInputElement).checked = cfg.imu.yaw.toUSBDevice !== false;
      (document.getElementById('yaw-usb-host') as HTMLInputElement).checked = cfg.imu.yaw.toUSBHost !== false;
      (document.getElementById('yaw-sensitivity') as HTMLInputElement).value = (cfg.imu.yaw.sensitivity || 1.0).toString();
      (document.getElementById('yaw-range') as HTMLInputElement).value = (cfg.imu.yaw.range || 90).toString();
    }
  }

  logInfo("Config loaded into UI.");
  statusDiv.textContent = "Config loaded into UI.";
  statusDiv.className = "status";
  return true;
}

// Export current config as JSON file
function exportConfig() {
  const statusDiv = document.getElementById("status") as HTMLDivElement;
  const config = buildConfigJson();
  if (!validateConfig(config)) {
    logError("Cannot export: invalid config.");
    statusDiv.textContent = "Cannot export: invalid config.";
    statusDiv.className = "status error";
    return;
  }
  const blob = new Blob([JSON.stringify(config, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = "rp2040-midi-config.json";
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  URL.revokeObjectURL(url);
  logInfo("Config exported as rp2040-midi-config.json");
}

// Handle file upload
function handleFileUpload(evt: Event) {
  const statusDiv = document.getElementById("status") as HTMLDivElement;
  const files = (evt.target as HTMLInputElement).files;
  if (!files || files.length === 0) return;
  const file = files[0];
  const reader = new FileReader();
  reader.onload = (e: ProgressEvent<FileReader>) => {
    try {
      const text = e.target?.result as string;
      const config = JSON.parse(text);
      if (applyConfigToUI(config)) {
        logInfo("Config file loaded and applied.");
      }
    } catch (err) {
      logError("Error parsing config file: " + err);
      statusDiv.textContent = "Error parsing config file.";
      statusDiv.className = "status error";
    }
  };
  reader.readAsText(file);
}

// Connect logic
async function connectSerial() {
  const statusDiv = document.getElementById("status") as HTMLDivElement;
  const versionDiv = document.getElementById("version") as HTMLDivElement;
  const sendBtn = document.getElementById("sendBtn") as HTMLButtonElement;
  statusDiv.textContent = "";
  try {
    if (!serialHandler.port) {
      await serialHandler.init();
      logInfo("Serial port opened");
      statusDiv.textContent = "Serial port connected.";
      statusDiv.className = "status";
      sendBtn.disabled = false;
    } else {
      statusDiv.textContent = "Already connected. Reading current config...";
      statusDiv.className = "status";
    }

    // Request config and version (whether newly connected or already connected)
    const readallCmd = JSON.stringify({ command: "READALL" }) + "\n";
    await serialHandler.write(readallCmd);
    logSent(readallCmd.trim());
    // Read response (expecting config JSON with version)
    for (let i = 0; i < 5; i++) {
      const resp = await serialHandler.readLine();
      if (resp) logRecv(resp);
      try {
        const config = JSON.parse(resp);
        if (config.version) {
          versionDiv.textContent = "Firmware Version: " + config.version;
        } else {
          versionDiv.textContent = "";
        }
        applyConfigToUI(config, true);
        statusDiv.textContent = "Config loaded from device.";
        break;
      } catch (e) {
        // Not a JSON config, skip
      }
    }
  } catch (err: any) {
    statusDiv.textContent = "Error: " + err;
    statusDiv.className = "status error";
    logError("" + err);
  }
}

// Send config logic
async function sendConfig() {
  const statusDiv = document.getElementById("status") as HTMLDivElement;
  if (!serialHandler.port) {
    statusDiv.textContent = "Serial port not connected. Please connect first.";
    statusDiv.className = "status error";
    logError("Serial port not connected.");
    return;
  }
  statusDiv.textContent = "";
  try {
    // Build and validate config
    const config = buildConfigJson();
    if (!validateConfig(config)) {
      statusDiv.textContent = "Invalid configuration: " + JSON.stringify(validateConfig.errors);
      statusDiv.className = "status error";
      logError("Invalid config: " + JSON.stringify(validateConfig.errors));
      return;
    }
    // Send config
    const jsonStr = JSON.stringify(config) + "\n";
    await serialHandler.write(jsonStr);
    logSent(jsonStr.trim());
    // Read response(s)
    for (let i = 0; i < 3; i++) { // Try to read up to 3 lines (status, echo, etc.)
      const resp = await serialHandler.readLine();
      if (resp) logRecv(resp);
      if (resp && resp.includes("Success")) break;
    }
    statusDiv.textContent = "Configuration sent successfully!";
    statusDiv.className = "status";
  } catch (err: any) {
    statusDiv.textContent = "Error: " + err;
    statusDiv.className = "status error";
    logError("" + err);
  }
}

// Calibrate IMU
async function calibrateIMU() {
  const statusDiv = document.getElementById("status") as HTMLDivElement;
  if (!serialHandler.port) {
    statusDiv.textContent = "Serial port not connected. Please connect first.";
    statusDiv.className = "status error";
    logError("Serial port not connected.");
    return;
  }
  statusDiv.textContent = "Calibrating IMU...";
  try {
    const command = { command: "CALIBRATE_IMU" };
    const cmdStr = JSON.stringify(command) + '\n';
    logSent(cmdStr);
    await serialHandler.write(cmdStr);
    
    // Read responses
    for (let i = 0; i < 10; i++) {
      const resp = await serialHandler.readLine();
      if (resp) logRecv(resp);
      if (resp && resp.includes("complete")) {
        statusDiv.textContent = "IMU calibration complete!";
        statusDiv.className = "status";
        break;
      }
    }
  } catch (err: any) {
    statusDiv.textContent = "Error: " + err;
    statusDiv.className = "status error";
    logError("" + err);
  }
}

// Attach event listeners after DOM is ready
document.addEventListener("DOMContentLoaded", () => {
  const connectBtn = document.getElementById("connectBtn") as HTMLButtonElement;
  const sendBtn = document.getElementById("sendBtn") as HTMLButtonElement;
  const uploadBtn = document.getElementById("uploadBtn") as HTMLButtonElement;
  const exportBtn = document.getElementById("exportBtn") as HTMLButtonElement;
  const calibrateBtn = document.getElementById("calibrate-imu") as HTMLButtonElement;
  const fileInput = document.getElementById("fileInput") as HTMLInputElement;

  connectBtn?.addEventListener("click", connectSerial);
  sendBtn?.addEventListener("click", sendConfig);
  uploadBtn?.addEventListener("click", () => fileInput.click());
  fileInput?.addEventListener("change", handleFileUpload);
  exportBtn?.addEventListener("click", exportConfig);
  calibrateBtn?.addEventListener("click", calibrateIMU);

  // Disable send button until connected
  sendBtn.disabled = true;
});

// Ensure serial port is closed when the page is closed or reloaded
window.addEventListener("beforeunload", async () => {
  try {
    await serialHandler.close();
  } catch (e) {
    // Ignore errors on unload
  }
});
