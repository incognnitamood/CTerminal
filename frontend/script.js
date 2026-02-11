/**
 * CTerminal Pro - Premium Web Terminal Interface
 * A high-end, ultra-readable terminal frontend for CTerminal backend
 */

// ============================================
// DOM Elements
// ============================================
const outputEl = document.getElementById('output');
const inputEl = document.getElementById('cmd-input');
const acEl = document.getElementById('autocomplete');
const ghostTextEl = document.getElementById('ghost-text');
const cwdEl = document.getElementById('prompt-cwd');
const terminalEl = document.getElementById('terminal');
const connectionStatusEl = document.getElementById('connection-status');
const commandCountEl = document.getElementById('command-count');
const statusDotEl = document.querySelector('.status-dot');

// ============================================
// State
// ============================================
let commandCount = 0;
let currentCwd = '/';
let autocompleteItems = [];
let selectedAutocompleteIndex = -1;
let isConnected = false;
let ghostSuggestion = '';

// Common commands for ghost suggestions
const commonCommands = [
  'mkdir', 'ls', 'cd', 'touch', 'write', 'read', 'rm', 'rmdir',
  'cat', 'pwd', 'cp', 'mv', 'rename', 'stat', 'search', 'chmod',
  'set', 'get', 'unset', 'listenv', 'undo', 'redo', 'history',
  'tree', 'export', 'import', 'help', 'clear'
];

// ============================================
// API Communication
// ============================================
async function runCommand(cmd) {
  try {
    const res = await fetch('http://localhost:3000/execute', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ command: cmd }),
    });
    
    if (!res.ok) throw new Error('Network error');
    
    const data = await res.json();
    setConnectionStatus(true);
    return data;
  } catch (error) {
    setConnectionStatus(false);
    throw error;
  }
}

function setConnectionStatus(connected) {
  isConnected = connected;
  if (connected) {
    statusDotEl.classList.add('connected');
    connectionStatusEl.textContent = 'Connected';
  } else {
    statusDotEl.classList.remove('connected');
    connectionStatusEl.textContent = 'Disconnected';
  }
}

// ============================================
// Output Rendering
// ============================================
function createCommandBlock(command, result) {
  const block = document.createElement('div');
  block.className = 'command-block';
  
  // Command line (prompt + command)
  const cmdLine = document.createElement('div');
  cmdLine.className = 'command-line';
  
  const prompt = document.createElement('span');
  prompt.className = 'command-prompt';
  prompt.textContent = `${currentCwd} $`;
  
  const cmdText = document.createElement('span');
  cmdText.className = 'command-text';
  cmdText.textContent = command;
  
  cmdLine.appendChild(prompt);
  cmdLine.appendChild(cmdText);
  block.appendChild(cmdLine);
  
  // Output content
  if (result) {
    // Determine output type based on result
    const hasError = result.stderr && !result.ok;
    const hasWarning = result.stderr && result.ok;
    const hasOutput = result.stdout && result.stdout.trim();
    
    // Show stdout
    if (hasOutput) {
      const outputDiv = document.createElement('div');
      outputDiv.className = 'output-content success';
      
      result.stdout.split('\n').forEach(line => {
        if (line.trim() || line === '') {
          const lineDiv = document.createElement('div');
          lineDiv.className = 'output-line';
          lineDiv.textContent = line;
          outputDiv.appendChild(lineDiv);
        }
      });
      
      block.appendChild(outputDiv);
    }
    
    // Show stderr (error or warning)
    if (result.stderr && result.stderr.trim()) {
      const errorDiv = document.createElement('div');
      errorDiv.className = `output-content ${hasWarning ? 'warning' : 'error'}`;
      
      result.stderr.split('\n').forEach(line => {
        if (line.trim()) {
          const lineDiv = document.createElement('div');
          lineDiv.className = 'output-line';
          lineDiv.textContent = line;
          errorDiv.appendChild(lineDiv);
        }
      });
      
      block.appendChild(errorDiv);
      
      // Shake animation for errors
      if (hasError) {
        triggerShake();
      }
    }
    
    // Update CWD
    if (result.cwd) {
      currentCwd = result.cwd;
      cwdEl.textContent = currentCwd;
    }
  }
  
  return block;
}

function appendCommandBlock(command, result) {
  const block = createCommandBlock(command, result);
  outputEl.appendChild(block);
  scrollToBottom();
  
  // Update command count
  commandCount++;
  commandCountEl.textContent = commandCount;
}

function appendErrorBlock(command, errorMessage) {
  const block = createCommandBlock(command, {
    ok: false,
    stdout: '',
    stderr: errorMessage,
    cwd: currentCwd
  });
  outputEl.appendChild(block);
  scrollToBottom();
  triggerShake();
}

function scrollToBottom() {
  requestAnimationFrame(() => {
    outputEl.scrollTop = outputEl.scrollHeight;
  });
}

function triggerShake() {
  terminalEl.classList.add('shake');
  setTimeout(() => {
    terminalEl.classList.remove('shake');
  }, 400);
}

// ============================================
// Welcome Banner
// ============================================
function showWelcomeBanner() {
  const banner = document.createElement('div');
  banner.className = 'welcome-banner';
  banner.innerHTML = `
    <div class="logo">CTerminal</div>
    <div class="tagline">Type 'help' to get started</div>
  `;
  outputEl.appendChild(banner);
}

// ============================================
// Autocomplete
// ============================================
function hideAutocomplete() {
  acEl.classList.remove('visible');
  acEl.innerHTML = '';
  autocompleteItems = [];
  selectedAutocompleteIndex = -1;
}

function showAutocomplete(items) {
  if (items.length === 0) {
    hideAutocomplete();
    return;
  }
  
  autocompleteItems = items;
  selectedAutocompleteIndex = -1;
  acEl.innerHTML = '';
  
  items.forEach((item, index) => {
    const div = document.createElement('div');
    div.className = 'autocomplete-item';
    div.setAttribute('role', 'option');
    div.setAttribute('aria-selected', 'false');
    
    const nameSpan = document.createElement('span');
    nameSpan.className = 'command-name';
    nameSpan.textContent = item;
    
    const hintSpan = document.createElement('span');
    hintSpan.className = 'hint';
    hintSpan.innerHTML = '<kbd>Tab</kbd> to select';
    
    div.appendChild(nameSpan);
    div.appendChild(hintSpan);
    
    div.onclick = () => selectAutocompleteItem(index);
    div.onmouseenter = () => highlightAutocompleteItem(index);
    
    acEl.appendChild(div);
  });
  
  acEl.classList.add('visible');
}

function highlightAutocompleteItem(index) {
  const items = acEl.querySelectorAll('.autocomplete-item');
  items.forEach((item, i) => {
    item.classList.toggle('selected', i === index);
    item.setAttribute('aria-selected', i === index ? 'true' : 'false');
  });
  selectedAutocompleteIndex = index;
}

function selectAutocompleteItem(index) {
  if (index >= 0 && index < autocompleteItems.length) {
    inputEl.value = autocompleteItems[index] + ' ';
    hideAutocomplete();
    updateGhostText();
    inputEl.focus();
  }
}

function navigateAutocomplete(direction) {
  if (autocompleteItems.length === 0) return;
  
  let newIndex = selectedAutocompleteIndex + direction;
  
  if (newIndex < 0) newIndex = autocompleteItems.length - 1;
  if (newIndex >= autocompleteItems.length) newIndex = 0;
  
  highlightAutocompleteItem(newIndex);
}

async function triggerAutocomplete() {
  const input = inputEl.value.trim();
  const parts = input.split(/\s+/);
  const prefix = parts[0] || '';
  
  if (!prefix) {
    hideAutocomplete();
    return;
  }
  
  try {
    const res = await runCommand('complete ' + prefix);
    const items = res.suggestions || [];
    
    if (items.length === 1 && items[0] === prefix) {
      // Exact match, add space and hide
      inputEl.value = items[0] + ' ';
      hideAutocomplete();
    } else if (items.length === 1) {
      // Single match, auto-complete
      inputEl.value = items[0] + ' ';
      hideAutocomplete();
    } else {
      showAutocomplete(items);
    }
  } catch (error) {
    hideAutocomplete();
  }
}

// ============================================
// Ghost Text (Inline Suggestions)
// ============================================
function updateGhostText() {
  const input = inputEl.value;
  
  if (!input) {
    ghostTextEl.textContent = '';
    ghostSuggestion = '';
    return;
  }
  
  // Find matching command
  const match = commonCommands.find(cmd => 
    cmd.startsWith(input.toLowerCase()) && cmd !== input.toLowerCase()
  );
  
  if (match && !input.includes(' ')) {
    ghostSuggestion = match;
    ghostTextEl.textContent = input + match.slice(input.length);
  } else {
    ghostSuggestion = '';
    ghostTextEl.textContent = '';
  }
}

function acceptGhostSuggestion() {
  if (ghostSuggestion) {
    inputEl.value = ghostSuggestion + ' ';
    ghostTextEl.textContent = '';
    ghostSuggestion = '';
  }
}

// ============================================
// Input Handling
// ============================================
function autoResizeInput() {
  inputEl.style.height = 'auto';
  inputEl.style.height = Math.min(inputEl.scrollHeight, 150) + 'px';
}

async function executeCommand(cmd) {
  if (!cmd.trim()) return;
  
  // Handle clear command locally
  if (cmd.trim() === 'clear') {
    outputEl.innerHTML = '';
    showWelcomeBanner();
    return;
  }
  
  try {
    const result = await runCommand(cmd);
    appendCommandBlock(cmd, result);
  } catch (error) {
    appendErrorBlock(cmd, 'Error: Unable to connect to backend server');
  }
}

// ============================================
// History Navigation (Backend)
// ============================================
async function navigateHistoryPrev() {
  try {
    const result = await runCommand('history_prev');
    if (result.stdout && result.stdout.trim()) {
      inputEl.value = result.stdout.trim();
      autoResizeInput();
      updateGhostText();
    }
  } catch (error) {
    // Silently fail
  }
}

async function navigateHistoryNext() {
  try {
    const result = await runCommand('history_next');
    if (result.stdout !== undefined) {
      inputEl.value = result.stdout.trim();
      autoResizeInput();
      updateGhostText();
    }
  } catch (error) {
    // Silently fail
  }
}

// ============================================
// Event Listeners
// ============================================
inputEl.addEventListener('keydown', async (e) => {
  // Handle autocomplete navigation
  if (acEl.classList.contains('visible')) {
    if (e.key === 'ArrowDown') {
      e.preventDefault();
      navigateAutocomplete(1);
      return;
    }
    if (e.key === 'ArrowUp') {
      e.preventDefault();
      navigateAutocomplete(-1);
      return;
    }
    if (e.key === 'Enter' && selectedAutocompleteIndex >= 0) {
      e.preventDefault();
      selectAutocompleteItem(selectedAutocompleteIndex);
      return;
    }
    if (e.key === 'Escape') {
      e.preventDefault();
      hideAutocomplete();
      return;
    }
  }
  
  // Main key handlers
  switch (e.key) {
    case 'Enter':
      if (!e.shiftKey) {
        e.preventDefault();
        const cmd = inputEl.value;
        inputEl.value = '';
        inputEl.style.height = 'auto';
        hideAutocomplete();
        ghostTextEl.textContent = '';
        await executeCommand(cmd);
      }
      break;
      
    case 'ArrowUp':
      if (!inputEl.value || inputEl.selectionStart === 0) {
        e.preventDefault();
        await navigateHistoryPrev();
      }
      break;
      
    case 'ArrowDown':
      if (!inputEl.value || inputEl.selectionStart === inputEl.value.length) {
        e.preventDefault();
        await navigateHistoryNext();
      }
      break;
      
    case 'Tab':
      e.preventDefault();
      if (ghostSuggestion) {
        acceptGhostSuggestion();
      } else if (selectedAutocompleteIndex >= 0) {
        selectAutocompleteItem(selectedAutocompleteIndex);
      } else {
        await triggerAutocomplete();
      }
      break;
      
    case 'Escape':
      hideAutocomplete();
      ghostTextEl.textContent = '';
      break;
  }
});

inputEl.addEventListener('input', () => {
  autoResizeInput();
  updateGhostText();
  
  // Hide autocomplete when typing
  if (acEl.classList.contains('visible')) {
    hideAutocomplete();
  }
});

inputEl.addEventListener('blur', () => {
  // Delay hiding to allow click events
  setTimeout(() => {
    if (!acEl.matches(':hover')) {
      hideAutocomplete();
    }
  }, 150);
});

// Click outside to hide autocomplete
document.addEventListener('click', (e) => {
  if (!acEl.contains(e.target) && e.target !== inputEl) {
    hideAutocomplete();
  }
});

// Window controls (cosmetic)
document.querySelector('.control.close')?.addEventListener('click', () => {
  if (confirm('Close terminal?')) {
    window.close();
  }
});

document.querySelector('.control.minimize')?.addEventListener('click', () => {
  terminalEl.style.transform = 'scale(0.95)';
  setTimeout(() => {
    terminalEl.style.transform = '';
  }, 200);
});

document.querySelector('.control.maximize')?.addEventListener('click', () => {
  terminalEl.classList.toggle('maximized');
});

// ============================================
// Initialization
// ============================================
async function init() {
  // Show welcome banner
  showWelcomeBanner();
  
  // Focus input
  inputEl.focus();
  
  // Test connection
  try {
    await runCommand('pwd');
    setConnectionStatus(true);
  } catch (error) {
    setConnectionStatus(false);
  }
  
  // Keep focus on input
  document.addEventListener('click', (e) => {
    if (!window.getSelection().toString()) {
      inputEl.focus();
    }
  });
}

// Start the terminal
init();

