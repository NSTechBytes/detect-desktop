# 🖥️ desktop-detector

A small Windows utility and Node.js wrapper that detects when the desktop becomes the foreground window (i.e., when all applications are minimized). Built in C++ and wrapped for easy use in Node.js.

---

## ⚙️ Features

- Detect when Windows desktop is shown or hidden (based on focus)
- Exposes a clean JavaScript API for use in Node.js
- Optional `--quiet` and `--log` CLI flags
- Useful for desktop automation, screen management, or shell enhancements

---

## 📦 Installation

```bash
npm install desktop-detector
````

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

You can also run the binary directly:

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
Initial state: Desktop is BACKGROUND
Listening for foreground changes; press Ctrl+C to exit.
*** Desktop is now FOREGROUND (shown)
*** Desktop is now BACKGROUND (apps shown)
```

---

## 📜 License

MIT © nstechbytes
