const outputEl = document.getElementById("output");
const inputEl = document.getElementById("cmd-input");
const acEl = document.getElementById("autocomplete");

const localHistory = [];
let histIndex = -1;

function appendLine(text, cls) {
  const div = document.createElement("div");
  div.textContent = text;
  if (cls) div.className = cls;
  outputEl.appendChild(div);
  outputEl.scrollTop = outputEl.scrollHeight;
}

function hideAutocomplete() {
  acEl.style.display = "none";
  acEl.innerHTML = "";
}

function showAutocomplete(items) {
  acEl.innerHTML = "";
  items.forEach((it) => {
    const div = document.createElement("div");
    div.className = "autocomplete-item";
    div.textContent = it;
    div.onclick = () => {
      inputEl.value = it + " ";
      hideAutocomplete();
      inputEl.focus();
    };
    acEl.appendChild(div);
  });
  if (items.length > 0) {
    acEl.style.display = "block";
  } else {
    hideAutocomplete();
  }
}

async function runCommand(cmd) {
  const res = await fetch("http://localhost:3000/execute", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ command: cmd }),
  });
  return res.json();
}

function renderResult(result) {
  if (result.stdout) {
    result.stdout.split("\n").forEach((line) => {
      if (line.trim()) appendLine(line, "stdout-line");
    });
  }
  if (result.stderr) {
    result.stderr.split("\n").forEach((line) => {
      if (line.trim()) appendLine(line, "stderr-line");
    });
  }
}

inputEl.addEventListener("keydown", (e) => {
  if (e.key === "Enter") {
    const cmd = inputEl.value;
    if (!cmd.trim()) return;
    if (cmd.trim() === "clear") {
      outputEl.innerHTML = "";
      inputEl.value = "";
      hideAutocomplete();
      return;
    }
    appendLine("$ " + cmd, "prompt-line");
    localHistory.push(cmd);
    histIndex = localHistory.length;
    inputEl.value = "";
    hideAutocomplete();
    runCommand(cmd)
      .then(renderResult)
      .catch(() => appendLine("Error talking to backend", "stderr-line"));
  } else if (e.key === "ArrowUp") {
    if (localHistory.length === 0) return;
    if (histIndex > 0) histIndex--;
    inputEl.value = localHistory[histIndex];
    e.preventDefault();
  } else if (e.key === "ArrowDown") {
    if (localHistory.length === 0) return;
    if (histIndex < localHistory.length - 1) {
      histIndex++;
      inputEl.value = localHistory[histIndex];
    } else {
      histIndex = localHistory.length;
      inputEl.value = "";
    }
    e.preventDefault();
  } else if (e.key === "Tab") {
    e.preventDefault();
    const current = inputEl.value.trim();
    const parts = current.split(/\s+/);
    const prefix = parts[0] || "";
    if (!prefix) return;
    runCommand("complete " + prefix)
      .then((res) => {
        const items = res.suggestions || [];
        if (items.length === 1) {
          inputEl.value = items[0] + " ";
          hideAutocomplete();
        } else {
          showAutocomplete(items);
        }
      })
      .catch(() => {});
  }
});

appendLine("DSA Web Terminal - connect to http://localhost:3000", "banner-line");
inputEl.focus();

