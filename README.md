# CTerminal - Educational Unix-like Shell

A fully-featured web-based terminal emulator with a **C backend** implementing a Unix-like shell entirely using custom in-memory data structures. Features a modern web UI that communicates with the backend via a Node.js bridge using JSON-RPC over HTTP.

Perfect for learning about **data structures**, **systems programming**, and **web application architecture**.

---

## Key Highlights

- **100% Custom Data Structures** - No standard library data structure APIs used
- **Virtual In-Memory Filesystem** - Complete Unix-like terminal experience
- **Full Undo/Redo Support** - Stack-based operation reversibility
- **Trie-Based Autocomplete** - Fast prefix-based command suggestions
- **Real-Time Web UI** - Dark theme terminal with JSON-based backend communication
- **Persistent State** - Export/import filesystem snapshots
- **Educational Codebase** - Written for maximum clarity and learning

## ✨ Implemented Features

- **Virtual filesystem (in-memory)**
  - Directories and files, rooted at `/`
  - Commands: `mkdir`, `ls`, `cd`, `touch`, `write`, `read`, `rm`, `rmdir`, `cat`, `pwd`
  - File permissions: simple read/write bits (`chmod <path> <r> <w>`)
- **Command history**
  - Last 100 commands, in-memory doubly linked list
  - `history` shows all stored commands
- **Undo / Redo**
  - Stack-based undo/redo of:
    - file/dir creation (`mkdir`, `touch`)
    - file content writes (`write`)
    - file deletes (`rm`)
    - file moves (`mv`)
  - Commands: `undo`, `redo`
- **Variables (environment-style)**
  - Hash map (chaining) with `set`, `get`, `unset`, `listenv`
- **Autocomplete**
  - Trie-based prefix autocomplete for command names (and can be extended to filenames)
  - Backend command: `complete <prefix>`
  - Frontend uses this on `Tab`
- **Search**
  - Recursive text search inside virtual filesystem: `search <start_path> <keyword>`
- **Help & logging**
  - `help` / `help <cmd>` for built-in help text
  - Circular queue logger for recent actions: `log`
- **State export / import**
  - Exports the **filesystem tree** (dirs, files, permissions, content) to a real file
  - Imports from that file and reconstructs the tree
  - Commands: `export <filename>`, `import <filename>`
- **Extra commands**
  - `cp <src> <dst>` (files; dirs created but shallow)
  - `mv <src> <dst>` (files only)
  - `tree [path]` (ASCII tree of current or given directory)
- **Web UI**
  - Dark “hacker” terminal look
  - Command input with history (arrow keys)
  - `clear` command to wipe screen (frontend)
  - Backend wired via HTTP bridge (`node bridge/server.js`)

---

## Architecture Overview

- **Backend (C, in `backend/`)**
  - **Process model**: single `terminal` / `terminal.exe` process
    - Reads one line from `stdin`
    - Tokenizes it
    - Executes command in memory
    - Prints a **single JSON line** to `stdout`
  - **Modules**
    - `utils.{c,h}`: basic string/memory helpers and a small dynamic buffer (`UBuffer`)
    - `filesystem.{c,h}`: tree-based virtual FS, search, permissions, export/import
    - `history.{c,h}`: doubly linked list of commands (max 100)
    - `stack.{c,h}`: dynamic array stack for `Operation` (undo/redo)
    - `hashmap.{c,h}`: hash map with chaining for variables
    - `parser.{c,h}`: quote/escape-aware tokenizer
    - `trie.{c,h}`: trie-based autocomplete
    - `logger.{c,h}`: circular log buffer
    - `commands.{c,h}`: maps parsed tokens → command implementations
    - `main.c`: stdin loop + JSON response encoder

- **Bridge (Node, in `bridge/server.js`)**
  - Spawns backend (`terminal.exe` on Windows, `./terminal` on Unix)
  - Maintains a persistent child process, communicates over `stdin`/`stdout`
  - HTTP API:
    - `POST /execute` with JSON `{ "command": "mkdir test" }`
    - Returns backend’s JSON response

- **Frontend (in `frontend/`)**
  - `index.html`: terminal layout (output area, prompt line, autocomplete dropdown)
  - `style.css`: dark theme, monospace, scrollbar/custom colors
  - `script.js`:
    - Sends commands to `http://localhost:3000/execute`
    - Renders `stdout` and `stderr` lines differently
    - Handles history navigation (up/down)
    - Tab → calls backend `complete` and shows suggestions
    - `clear` wipes the screen and keeps input focused

---

## Data Structures in Detail

- **Virtual filesystem (`TreeNode` in `filesystem.h`)**
  - `name` (string)
  - `type` (`NODE_FILE` or `NODE_DIR`)
  - `content` / `content_size` / `content_capacity` for files
  - `perms_read` / `perms_write` bits
  - `children` dynamic array for directories
  - `parent` pointer
  - Operations:
    - `fs_mkdir`, `fs_touch`, `fs_ls`, `fs_cd`, `fs_pwd`
    - `fs_write`, `fs_read`, `fs_rm`, `fs_rmdir`
    - `fs_search` with DFS and line-by-line keyword search
    - `fs_chmod`, `fs_copy`, `fs_move`
    - `fs_export_to_file`, `fs_import_from_file`

- **History (`HistoryList` in `history.h`)**
  - Doubly linked list with head/tail, up to 100 commands
  - `history_add` appends; `history_get_all` returns a copied array

- **Undo/Redo (`OpStack` in `stack.h`)**
  - Dynamic array of `Operation`:
    - `OperationType` enum:
      - `OP_CREATE_FILE`, `OP_DELETE_FILE`, `OP_CREATE_DIR`, `OP_DELETE_DIR`
      - `OP_WRITE_FILE`, `OP_MOVE`
    - `path`, `old_content`, `new_content`
  - Undo reverses operations; redo reapplies them.

- **Variables (`HashMap` / `VarEntry` in `hashmap.h`)**
  - `buckets` array of singly linked lists
  - djb2 hash function
  - Operations: `hm_init`, `hm_set`, `hm_get`, `hm_unset`, `hm_list`

- **Parser (`TokenArray` in `parser.h`)**
  - Manual state machine for:
    - normal text
    - quotes (`"..."`)
    - backslash escapes (`\"`, `\\`, etc.)

- **Trie (`TrieNode` in `trie.h`)**
  - 256-way array of child pointers per node (ASCII)
  - `trie_insert`, `trie_complete` (prefix → suggestions)

---

## JSON Protocol

For every input line, backend prints **one JSON object on a single line**:

```json
{
  "ok": true,
  "stdout": "command output here",
  "stderr": "",
  "cwd": "/current/directory",
  "suggestions": ["opt1", "opt2"]
}
```

- **ok**: `true` if command succeeded, `false` on error.
- **stdout**: plain text output (may contain `\n`).
- **stderr**: error message if any; empty string otherwise.
- **cwd**: current working directory after the command.
- **suggestions**: only non-empty for `complete` calls (autocomplete).

The Node bridge simply relays this JSON to the browser.

---

## Commands

### Filesystem

- `mkdir <dir>`: create directory.
- `ls [path]`: list directory contents.
- `cd <path>`: change directory (no arg → `/`).
- `touch <file>`: create empty file if not present.
- `write <file> <text...>`: overwrite file contents with joined text.
- `read <file>`: print raw file contents.
- `rm <file>`: remove file (respects write permission).
- `rmdir <dir>`: remove empty directory (no undo snapshot).
- `cat <file>`: print file with line numbers.
- `pwd`: show current working directory.
- `cp <src> <dst>`: copy file (and create target dir shallowly).
- `mv <src> <dst>`: move file; records undo/redo.
- `tree [path]`: show directory tree.

### Variables

- `set <key> <value>`: set variable.
- `get <key>`: get value.
- `unset <key>`: delete variable.
- `listenv`: list all variables as `KEY=VALUE`.

### History / Undo / Redo

- `history`: show last commands.
- `undo`: undo last operation (create/write/delete file, create dir, move file).
- `redo`: redo last undone operation.

### Search / Help / Logging

- `search <path> <keyword>`: search files recursively for keyword.
- `help`: show command summary.
- `help <cmd>`: show help line for that command (prefix match).
- `log`: print circular log entries.

### Permissions / State

- `chmod <path> <r> <w>`:
  - `r` and `w` are `0` or `1` (e.g. `chmod a.txt 1 0` = read-only).
- `export <filename>`:
  - Write filesystem state to a real file (only OS I/O used).
- `import <filename>`:
  - Clear current FS and load from exported file.

### Frontend-only

- `clear`: clear terminal output (implemented in JS only).

---

## Build & Run

### Prerequisites

- **C compiler** (e.g. `gcc`)
- **Node.js** (for the bridge)

### Windows

From project root (`c:\Users\sujat\OneDrive\Desktop\dsa_project`):

```powershell
.\build.bat
```

This produces `terminal.exe` in the project root.

Start the bridge:

```powershell
node bridge\server.js
```

Then open `frontend\index.html` in your browser.  
Type a command (e.g. `mkdir test`) and press Enter; results come from the C backend.

### Linux / macOS

From project root:

```bash
make          # builds ./terminal
node bridge/server.js
```

Open `frontend/index.html` in a browser.

---

## Design Constraints & Notes

- **No standard data-structure APIs**: all lists, stacks, tries, hash maps are written manually.
- **Filesystem isolation**: the backend never touches the real OS filesystem except for:
  - `export` / `import` files, using plain `fopen` / `fprintf` / `fgets` / `fclose`.
- **Single-process, single-threaded**: good for deterministic grading and easy reasoning.
- **Undo/Redo limitations**:
  - Full support for files and simple directory creation.
  - `rmdir` does **not** snapshot subtrees (by design – optional future work).
- **Memory**:
  - Core structures are freed on `fs_clear` and process exit.
  - Suggestions and temporary strings are freed immediately after use.

---

## Possible Future Improvements

- Recursive directory copy in `cp` (full `cp -r` behavior).
- Export/import of variables and history for complete session restore.
- `trie_free` and more aggressive freeing for perfect leak-free runs under valgrind.
- Richer `tree` rendering and autocomplete UI.
- WebSocket-based bridge for streaming / real-time output.

