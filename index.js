const { spawn } = require('child_process');
const path = require('path');

const exePath = path.join(__dirname, 'bin', 'Release','detect-desktop.exe');

function startDesktopDetector({ quiet = false, log = false } = {}) {
  const args = [];
  if (quiet) args.push('--quiet');
  if (log) args.push('--log');

  const proc = spawn(exePath, args);

  proc.stdout.on('data', (data) => {
    console.log(`[desktop-detector]: ${data.toString().trim()}`);
  });

  proc.stderr.on('data', (data) => {
    console.error(`[desktop-detector-error]: ${data.toString().trim()}`);
  });

  proc.on('exit', (code) => {
    console.log(`[desktop-detector]: exited with code ${code}`);
  });

  return proc; 
}

module.exports = { startDesktopDetector };
