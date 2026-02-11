const http = require("http");
const { spawn } = require("child_process");
const path = require("path");

const root = path.join(__dirname, "..");
const bin = process.platform === "win32" ? path.join(root, "terminal.exe") : path.join(root, "terminal");

let backendError = null;
const child = spawn(bin, {
  cwd: root,
  stdio: ["pipe", "pipe", "inherit"],
  windowsHide: true,
});

let buffer = "";
const pending = [];

child.on("error", (err) => {
  backendError = err;
  while (pending.length) {
    const p = pending.shift();
    p.reject(err);
  }
});

child.stdout.on("data", (chunk) => {
  buffer += chunk.toString("utf8");
  let idx;
  while ((idx = buffer.indexOf("\n")) >= 0) {
    const line = buffer.slice(0, idx);
    buffer = buffer.slice(idx + 1);
    const req = pending.shift();
    if (!req) continue;
    try {
      const json = JSON.parse(line);
      req.resolve(json);
    } catch (e) {
      req.reject(e);
    }
  }
});

child.on("exit", () => {
  while (pending.length) {
    const p = pending.shift();
    p.reject(new Error("backend exited"));
  }
});

function sendCommand(cmd) {
  return new Promise((resolve, reject) => {
    if (backendError) return reject(backendError);
    pending.push({ resolve, reject });
    child.stdin.write(cmd + "\n");
  });
}

const server = http.createServer((req, res) => {
  if (req.method === "OPTIONS") {
    res.writeHead(204, {
      "Access-Control-Allow-Origin": "*",
      "Access-Control-Allow-Methods": "POST, OPTIONS",
      "Access-Control-Allow-Headers": "Content-Type",
    });
    return res.end();
  }
  if (req.method !== "POST" || req.url !== "/execute") {
    res.writeHead(404, { "Access-Control-Allow-Origin": "*" });
    return res.end("not found");
  }
  let body = "";
  req.on("data", (chunk) => (body += chunk.toString("utf8")));
  req.on("end", async () => {
    try {
      const data = JSON.parse(body || "{}");
      const cmd = (data.command || "").trim();
      if (!cmd) {
        res.writeHead(400, { "Access-Control-Allow-Origin": "*" });
        return res.end(JSON.stringify({ ok: false, stderr: "empty command" }));
      }
      const result = await sendCommand(cmd);
      res.writeHead(200, {
        "Content-Type": "application/json",
        "Access-Control-Allow-Origin": "*",
      });
      res.end(JSON.stringify(result));
    } catch (e) {
      res.writeHead(500, { "Access-Control-Allow-Origin": "*" });
      res.end(
        JSON.stringify({
          ok: false,
          stderr: backendError
            ? "backend spawn failed: " + String(backendError.message || backendError)
            : "server error",
        })
      );
    }
  });
});

server.listen(3000, () => {
  console.log("Bridge listening on http://localhost:3000");
});

