# 🖥️ desktop-detector

A small Windows utility and Node.js wrapper that detects when the desktop becomes the foreground window (i.e., when all applications are minimized).

This version improves detection by distinguishing between a true "Show Desktop" (triggered via Win+D or the taskbar button) and a simple click on the desktop:

* **Show Desktop** is recognized when

  * Any window minimize-start event occurs within a 1-second window before desktop activation (i.e., burst ≥ 1),
  * Or via the Show Desktop button or Win+D hotkey.
* **Plain Click** on the desktop is ignored as a Show Desktop, and only logs a simple "shown" state.

---

## ⚙️ Features

* Detect when Windows desktop is shown or hidden (based on focus)
* Distinguish "Show Desktop" (key/button) from ordinary clicks
* Exposes a clean JavaScript API for use in Node.js
* Optional `--quiet` and `--log` CLI flags

---

## 📦 Installation

```bash
npm install desktop-detector
```

Or clone it:

```bash
git clone https://github.com/nstechbytes/desktop-detector.git
cd desktop-detector
npm install
```

---

## 🚀 Usage

### In Node.js:

```js
const { startDesktopDetector } = require('desktop-detector');

// Start monitoring
const proc = startDesktopDetector({
  quiet: false, // show console output
  log: true     // also log to `detect-desktop.log`
});

// Optional: stop after 10 seconds
setTimeout(() => {
  proc.kill();
  console.log("Stopped detector.");
}, 10000);
```

---

## 🧰 CLI Options

Run the binary directly:

```bash
./bin/detect-desktop.exe [--quiet] [--log] [--help]
```

| Flag      | Description                         |
| --------- | ----------------------------------- |
| `--quiet` | Suppresses console output           |
| `--log`   | Logs output to `detect-desktop.log` |
| `--help`  | Displays usage information          |

---

## 🧪 Example Output

```
[2025-05-24 16:00:00] Initial state: Desktop is BACKGROUND
[2025-05-24 16:00:00] Listening for Show Desktop; press Ctrl+C to exit.
[2025-05-24 16:01:10] *** Desktop is now FOREGROUND (Show Desktop)
[2025-05-24 16:01:12] *** Desktop is now BACKGROUND (apps shown)
[2025-05-24 16:02:00] *** Desktop is now FOREGROUND (shown)
```

---

## 📜 License

MIT © nstechbytes
