# EPL v1.8 — System Runtime & NOVA Integration

> *"The language that wrote itself into a system."*
> EPL began as a scripting language for humans. In v1.8 it becomes an operating-layer —
> capable of controlling processes, owning sockets, and orchestrating AI services natively.

**Files patched:** `token.h`, `lexer.c`, `ast.h`, `parser.c`, `interpreter.c`, `token.c`
**New files:** `nova_executioner.EP`, `nova_brain.EP`
**New services:** `nova-executioner-epl.service`, `nova-brain-epl.service`

Recompile on Linux (Arch):
```bash
cd ~/Desktop/EPL/build
make
# or manually:
gcc -Wall -std=c11 -o epl main.c lexer.c token.c ast.c parser.c \
    interpreter.c repl.c codegen.c binding.c describe.c scene_gen.c json.c -lm
```

---

## Overview — What Changed

EPL v1.8 is the **System Runtime** release. Prior versions gave EPL expressive
power over data, logic, collections, graphs, and frontend rendering. v1.8 adds
a POSIX-native runtime layer: file I/O, shell capture, environment access,
Unix socket IPC (client and server), and lightweight HTTP — implemented
directly in C so EPL scripts run at native speed with zero Python overhead.

This release is the foundation for **Project NOVA** — an AI voice assistant
whose automation and orchestration daemons are being progressively rewritten
from Python into EPL. The philosophy: if NOVA can describe what it wants in
English, it should be able to run it in English.

```
v1.0  Core language, interpreter
v1.1  Smart collections
v1.2  String ops, number formatting, type checking
v1.3  Graph data structures
v1.4  EPF frontend engine
v1.5  Semantic indexes (.EPD)
v1.6  3D canvas engine
v1.7  HTML/DOM interpreter, multi-page manifest
v1.8  ← YOU ARE HERE
      System Runtime: files, sockets, HTTP, shell, IPC
      NOVA daemon migration begins
```

---

## 1. New Language Primitives

### 1.1 File I/O

Read, write, append, check existence, delete — paths support `~` expansion.

```epl
-- Read entire file into a string
set content as read file "~/nova/logs/chat.jsonl"

-- Overwrite (or create) a file
write content to file "/tmp/output.txt"

-- Append a line (for JSONL logs)
append entry to file "~/nova/logs/exec_history.jsonl"

-- Check if a file exists (returns true/false)
set exists as file exists "/tmp/nova_executioner.sock"

-- Delete a file
delete file "/tmp/old.sock"
```

**C implementation:** `fopen`/`fread`/`fputs` with `~` → `$HOME` expansion.
`file exists` uses POSIX `access(path, F_OK)`.

> **Changed in v1.9:** failed reads, writes, appends and deletes raise
> catchable errors carrying the underlying `strerror` reason. In v1.8 they
> returned `""` or silently did nothing, which `attempt` could not detect.

---

### 1.2 Shell Command Capture

Run any shell command and capture its standard output as a string.
The existing `execute` keyword runs a command silently (fire-and-forget).
`run` is the new expression form that returns stdout.

```epl
-- Capture stdout as a string value
set volume as run "wpctl get-volume @DEFAULT_AUDIO_SINK@"
set window_json as run "hyprctl activewindow -j"
set cpu as run "grep 'cpu MHz' /proc/cpuinfo | head -1"

show "Current volume: " plus volume
```

**C implementation:** `popen(cmd, "r")` + `fread` into a heap buffer.
Trailing newline is stripped.

> **Changed in v1.9:** a non-zero exit status now raises a catchable error
> instead of silently yielding `""`. Captured stdout is still returned.
> ```epl
> attempt
>     set out as run "some-command-that-fails"
> if fails then
>     show "command failed: " plus error message
> !
> ```

---

### 1.3 Environment Variables

```epl
set api_key as get env "N8N_API"
set home as get env "HOME"
set display as get env "WAYLAND_DISPLAY"

if api_key is "" then
    show "Warning: N8N_API not set"
!
```

**C implementation:** `getenv(name)` — returns null value if the variable is unset.

---

### 1.4 Time & Sleep

```epl
-- Current timestamp (ISO 8601)
set ts as now
show "Started at: " plus ts

-- Pause execution (integer seconds)
sleep 2
```

`now` returns a string like `"2026-06-06T02:31:00"`.
**C implementation:** `time(NULL)` + `strftime`.

---

### 1.5 Unix Socket Client (IPC)

Send a JSON string to a Unix domain socket and receive a JSON response.
This is the backbone of NOVA's inter-daemon communication.

```epl
-- Send JSON to a socket, get response back
set payload as "{\"priority\":1,\"text\":\"Online, sir.\"}"
set reply as send json payload to socket "/tmp/nova_tts.sock"

-- Check success
set ok as field "status" from json reply
```

**C implementation:** `socket(AF_UNIX)` → `connect()` → `send(json + "\n")`
→ `recv()` until newline → return response string. Closes the connection
after each exchange (stateless, mirrors the Python pattern exactly).

---

### 1.6 HTTP POST (for Ollama & N8N)

Send a JSON body to an HTTP endpoint and receive the response body.
No external libraries — raw TCP socket, minimal HTTP/1.1 parser.

```epl
-- Call Ollama locally
set body as "{\"model\":\"llama3.2:1b\",\"prompt\":\"" plus query plus "\",\"stream\":false}"
set response as post json body to "http://localhost:11434/api/generate"
set answer as field "response" from json response

-- Trigger N8N webhook
set search_body as "{\"query\":\"" plus query plus "\",\"mode\":\"search\"}"
set web_result as post json search_body to "http://localhost:5678/webhook/nova-web-search"
set summary as field "summary" from json web_result
```

**C implementation:** Parses `host`, `port`, and `path` from the URL string.
`socket(AF_INET)` → `connect()` → writes HTTP/1.1 `POST` with `Content-Type: application/json`
→ reads response → strips HTTP headers → returns body string.
Default timeout: 30 seconds.

---

### 1.7 JSON Field Extraction

Extract a named field from a raw JSON string. Returns the value as a string.
Numbers and booleans are returned as their string representations.

```epl
set json_str as "{\"success\":true,\"stdout\":\"Volume 0.72\",\"tier\":1}"

set ok     as field "success" from json json_str
set output as field "stdout"  from json json_str
set tier   as field "tier"    from json json_str

show "Success: " plus ok
show "Output: "  plus output
```

> ### ⚠️ Superseded by v1.9
>
> The v1.8 implementation was a string scanner that stopped at the first `,`,
> `}` or `]`. On nested input it returned a **truncated, malformed fragment**:
>
> ```epl
> field "user" from json "{\"user\":{\"name\":\"tathya\",\"tier\":2}}"
> -- v1.8 returned:  {"name":"tathya      ← malformed, silently wrong
> ```
>
> It did **not** handle nesting depth, contrary to what this section
> originally claimed. v1.9 replaces it with a real recursive-descent parser
> (`json.c`) and a native `VAL_MAP` type. `field ... from json` keeps working
> on raw strings, but now returns correct values and supports dotted paths:
>
> ```epl
> set doc  as parse json response
> set name as field "user.name" from doc
> set body as to json doc
> ```
>
> See `EPL_v1.9_Release.md` for the full JSON reference.

---

### 1.8 Unix Socket Server (Daemon Mode)

Run an EPL activity as a Unix socket server. This is how EPL replaces
Python's `threading` + `socket.accept()` daemon pattern.

```epl
describe handle_connection as "Processes one incoming IPC request"
to handle_connection taking request
    set action as field "action" from json request

    if action is "ping" then
        reply with "{\"pong\":true}"
        return
    !

    -- ... process the request ...
    set result as "{\"success\":true}"
    reply with result
!

-- This line starts the daemon — it loops forever:
serve on socket "/tmp/nova_myservice.sock" using handle_connection
```

**How it works:**
1. Unlinks stale socket file if present
2. `socket(AF_UNIX)` + `bind()` + `chmod(0600)` + `listen(16)`
3. Accept loop: `accept()` → read full message → bind it to `"request"` → call activity
4. Activity uses `reply with` to write the response back to the caller
5. Connection closed; loop repeats

`reply with` sends the value followed by a newline to the current connection.
Calling `reply with` outside a `serve` handler is a no-op.

---

## 2. Keyword Reference (v1.8 additions)

| Syntax | Category | Returns |
|--------|----------|---------|
| `read file "path"` | File I/O | string |
| `write VALUE to file "path"` | File I/O | — |
| `append VALUE to file "path"` | File I/O | — |
| `file exists "path"` | File I/O | boolean |
| `delete file "path"` | File I/O | — |
| `run "cmd"` | Shell | string (stdout) |
| `get env "VAR"` | System | string |
| `now` | Time | string (ISO 8601) |
| `sleep N` | Time | — |
| `send json VALUE to socket "path"` | IPC | string (response) |
| `post json VALUE to "url"` | HTTP | string (body) |
| `field "key" from json VALUE` | JSON | string |
| `serve on socket "path" using ACTIVITY` | IPC Server | — (loops forever) |
| `reply with VALUE` | IPC Server | — |

---

## 3. New AST Nodes (for maintainers / LLM consumers)

| NodeType | Mode | Data fields |
|----------|------|-------------|
| `NODE_READ_FILE` | expression | `path_expr` |
| `NODE_WRITE_FILE` | statement | `value_expr`, `path_expr` |
| `NODE_APPEND_FILE` | statement | `value_expr`, `path_expr` |
| `NODE_FILE_EXISTS` | expression | `path_expr` |
| `NODE_DELETE_FILE` | statement | `path_expr` |
| `NODE_RUN_CMD` | expression | `cmd_expr` |
| `NODE_GET_ENV` | expression | `var_expr` |
| `NODE_NOW` | expression | *(none)* |
| `NODE_SLEEP` | statement | `duration_expr` |
| `NODE_SEND_SOCKET` | expression | `json_expr`, `path_expr` |
| `NODE_POST_HTTP` | expression | `json_expr`, `url_expr` |
| `NODE_JSON_FIELD` | expression | `key_expr`, `json_expr` |
| `NODE_SERVE_SOCKET` | statement | `path_expr`, `activity[MAX_TOKEN_LEN]` |
| `NODE_REPLY_WITH` | statement | `value_expr` |

---

## 4. New Tokens (for maintainers / LLM consumers)

Single-word additions to `keyword_table[]` in `lexer.c`:

```
read, write, append, file, delete, run, env, now, sleep, socket, serve, post, fetch, json, reply, using, exists, get
```

Compound phrases added to `phrase_table[]` (longest-first):

```
"serve on socket"   → TOKEN_SERVE_SOCK_PH
"send json"         → TOKEN_SEND_JSON_PH
"post json"         → TOKEN_POST_JSON_PH
"field from json"   → TOKEN_FIELD_JSON_PH
"read file"         → TOKEN_READ_FILE_PH
"write to file"     → TOKEN_WRITE_FILE_PH
"append to file"    → TOKEN_APPEND_FILE_PH
"delete file"       → TOKEN_DELETE_FILE_PH
"file exists"       → TOKEN_FILE_EXISTS_PH
"get env"           → TOKEN_GET_ENV_PH
"reply with"        → TOKEN_REPLY_WITH_PH
```

---

## 5. Project NOVA — EPL Migration Roadmap

NOVA is a locally-running AI voice assistant (Arch Linux / Hyprland) built on
seven resident systemd daemons communicating via Unix domain sockets.

### Daemon Migration Status

| Daemon | File | Status | Reason |
|--------|------|--------|--------|
| **nova-executioner** | `executioner.py` | 🔄 **Migrating** | Pure logic — no ML dependency |
| **nova-brain** | `brain.py` | 🔄 **Migrating** | HTTP + socket orchestration |
| **nova-stt** | `stt_daemon.py` | ⛔ Stays Python | `openwakeword` + `faster-whisper` |
| **nova-tts** | `tts_daemon.py` | ⛔ Stays Python | `Kokoro` + `piper-tts` |
| **nova-router** | `router_bridge.py` | ⛔ Stays Python | DeBERTa intent classifier |
| **nova-rag** | `rag_daemon.py` | 🔲 Planned v1.9 | SQLite FTS5 — feasible in C |
| **nova-watchdog** | `watchdog.py` | 🔲 Planned v1.9 | File monitoring — feasible |

> The STT, TTS, and Router daemons depend on Python ML libraries with no
> viable C equivalent at this scale. Python's ML ecosystem is unmatched
> for inference tasks. EPL takes over everything that is *logic*, not *inference*.

### EPL Brain Retraining

With the migration complete, NOVA's brain (Ollama/Mistral) will be fine-tuned
to understand EPL as its native code generation target:

- When NOVA needs to perform an action, it generates an `.EP` snippet
- The EPL snippet calls `send json`, `run`, `write`, `post json`, etc.
- The executioner daemon (`nova_executioner.EP`) validates and runs it
- Results are logged back to `exec_history.jsonl` via `append ... to file`

This closes the loop: **English in → EPL out → NOVA executes → logs in EPL**.

The brain will also generate `.EPD` describe blocks for new service definitions
it creates, keeping the semantic index up to date automatically.

---

## 6. NOVA Executioner — EPL Implementation

`~/nova/executioner/nova_executioner.EP`

```epl
describe handle_connection as "Processes one IPC request from the brain or router"
to handle_connection taking request
    set action as field "action" from json request

    -- Ping / health check
    if action is "ping" then
        reply with "{\"status\":\"ok\",\"service\":\"nova-executioner-epl\"}"
        return
    !

    -- Execute
    set script   as field "script"     from json request
    set narration as field "narration" from json request
    set ops       as field "operations" from json request

    -- Safety gate — banned patterns
    if script contains "rm -rf /" then
        reply with "{\"error\":\"Banned operation\",\"blocked\":true}"
        return
    !
    if script contains "mkfs" then
        reply with "{\"error\":\"Banned operation\",\"blocked\":true}"
        return
    !

    -- Narrate if tier 2
    if narration is not "" then
        set tts_payload as "{\"priority\":1,\"text\":\"" plus narration plus "\"}"
        set tts_reply as send json tts_payload to socket "/tmp/nova_tts.sock"
    !

    -- Write script to workspace
    set ts as now
    set script_path as "/home/tathya/nova/workspace/nova_" plus ts plus ".py"
    write script to file script_path

    -- Run it
    set output as run "python3 " plus script_path
    delete file script_path

    -- Build result JSON
    set result as "{\"success\":true,\"stdout\":\"" plus output plus "\",\"duration_ms\":0}"

    -- Log to history
    set log_entry as result plus "\n"
    append log_entry to file "/home/tathya/nova/logs/exec_history.jsonl"

    reply with result plus "\n"
!

serve on socket "/tmp/nova_executioner.sock" using handle_connection
```

---

## 7. NOVA Brain Orchestrator — EPL Implementation (Partial)

`~/nova/brain/nova_brain.EP`

```epl
describe handle_brain_request as "Orchestrates LLM call and tool dispatch"
to handle_brain_request taking request
    set query  as field "query"  from json request
    set intent as field "intent" from json request

    -- Get system snapshot via shell
    set volume  as run "wpctl get-volume @DEFAULT_AUDIO_SINK@"
    set window  as run "hyprctl activewindow -j"
    set cpu     as run "grep -m1 'cpu MHz' /proc/cpuinfo"

    -- Build Ollama prompt
    set prompt as "Volume: " plus volume plus " | Window: " plus window
    set prompt as prompt plus " | Query: " plus query

    set body as "{\"model\":\"llama3.2:1b\",\"prompt\":\"" plus prompt plus "\",\"stream\":false}"
    set response as post json body to "http://localhost:11434/api/generate"
    set spoken as field "response" from json response

    -- Send to TTS
    set tts_msg as "{\"priority\":1,\"text\":\"" plus spoken plus "\"}"
    set tts_ok as send json tts_msg to socket "/tmp/nova_tts.sock"

    -- Log exchange
    set log_line as "{\"query\":\"" plus query plus "\",\"response\":\"" plus spoken plus "\"}\n"
    append log_line to file "/home/tathya/nova/logs/chat.jsonl"

    reply with "{\"spoken\":\"" plus spoken plus "\"}"
!

serve on socket "/tmp/nova_brain.sock" using handle_brain_request
```

---

## 8. Full Example: System Snapshot Tool

```epl
describe nova_snapshot as "Reads live system state and returns JSON"
to nova_snapshot
    set volume  as run "wpctl get-volume @DEFAULT_AUDIO_SINK@"
    set window  as run "hyprctl activewindow -j | python3 -c \"import sys,json; d=json.load(sys.stdin); print(d.get('title',''))\""
    set cpu     as run "top -bn1 | grep 'Cpu(s)' | awk '{print $2}'"
    set ts      as now

    set snapshot as "{\"time\":\"" plus ts
    set snapshot as snapshot plus "\",\"volume\":\"" plus volume
    set snapshot as snapshot plus "\",\"window\":\"" plus window
    set snapshot as snapshot plus "\",\"cpu\":\"" plus cpu plus "\"}"

    return snapshot
!

set snap as nova_snapshot
show snap
```

Output:
```json
{"time":"2026-06-06T02:31:00","volume":"Volume 0.72","window":"foot","cpu":"2.3"}
```

---

## 9. .EPD Integration for NOVA Services

NOVA service files (`.EP`) generate `.EPD` semantic indexes via:

```bash
./epl --describe ~/nova/executioner/nova_executioner.EP
```

This outputs `nova_executioner.EPD` — a token-efficient index of every
`describe` block. The brain model reads this before generating any new code,
ensuring it understands the service contract before writing to it.

Example `.EPD` output for the executioner:
```
[activity] handle_connection (line 2–38)
  "Processes one IPC request from the brain or router"
  takes: request (string — raw JSON from socket)
  uses: send json, run, write, append, reply with
```

---

## 10. Compatibility Notes

- All v1.0–v1.7 syntax is fully backward compatible.
- `execute` (the silent fire-and-forget command keyword) is unchanged.
- `run` is a new *expression* keyword (not a statement). It cannot conflict
  with existing identifiers named `run` because EPL resolves multi-word
  phrases greedily before falling back to identifiers.
- `reply with` is only meaningful inside a `serve on socket` handler.
  Used elsewhere it is a silent no-op.
- `post json` and `send json` are blocking calls. Long Ollama completions
  (Mistral 7B can take 30s) will block the serve loop. Async dispatch
  (`post json ... in background`) remains a post-launch consideration.

### Corrections applied in v1.9

Two things this document described did not work as written before v1.9:

1. **`taking` did not exist.** `to handle_connection taking request` registered
   an activity literally *named* `"handle_connection taking request"`, and
   arguments were bound to six hardcoded globals `x, y, z, a, b, c`. Every
   `taking` example below was unrunnable. v1.9 implements real named
   parameters, so the examples in §6, §7 and §8 now work as printed.
2. **`field ... from json` corrupted nested data** — see the notice in §1.7.

Both are fixed; the NOVA service examples in this document are now verified
against the interpreter.

---

## Changelog

| Version | Focus | Key Addition |
|---------|-------|-------------|
| v1.0 | Core | Interpreter, activities, loops, conditionals |
| v1.1 | Data | Smart collections, where filter |
| v1.2 | String | String ops, number formatting, type checks |
| v1.3 | Graphs | Graph data structure, BFS routing |
| v1.4 | Frontend | EPF engine, Chart.js, HTML dashboards |
| v1.5 | Semantics | `.EPD` index, `describe` blocks |
| v1.6 | 3D | `canvas3d`, orbital rings, Three.js output |
| v1.7 | DOM | HTML interpreter, multi-page manifest |
| **v1.8** | **System** | **File I/O, shell, sockets, HTTP, JSON, IPC daemon** |
