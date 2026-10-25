# EPL v1.9 — Core Integrity Release

> *"A language you can trust is a language you can build on."*
> v1.9 adds almost no new surface area. It makes the surface EPL already
> advertised actually work — the release that had to land before a module
> system, a package manager, or a public launch could mean anything.

**Status:** implemented and tested
**New files:** `json.c`, `json.h`, `tests/`
**Files changed:** `token.h`, `lexer.c`, `lexer.h`, `ast.h`, `parser.c`,
`parser.h`, `interpreter.c`, `interpreter.h`, `main.c`, `Makefile`

```bash
cd build && make && make test
```

Every code example in this document has been run against the interpreter.

---

## 0. Why this release exists

An audit of the v1.8 tree against its own documentation found headline
features that were documented but never implemented, and implemented features
that failed silently instead of erroring.

| Claim | v1.8 reality | v1.9 |
|-------|--------------|------|
| `to handler taking request` | `taking` was not a token; the whole line became the activity **name** | ✅ real named parameters |
| Activities have parameters | bound positionally to six hardcoded globals `x,y,z,a,b,c`, max 6 | ✅ named, unlimited |
| `attempt` / `if fails` catches errors | `runtime_error()` was called **once** in the tree (division by zero) | ✅ ~15 raise sites |
| `read file` on a bad path is catchable | returned `""` silently | ✅ raises with `strerror` |
| `field from json` "handles nesting" | truncated at the first comma, returned malformed JSON | ✅ real parser + `VAL_MAP` |
| Unknown activity | returned `null`, indistinguishable from success | ✅ raises |
| `repeat N times with i` | **parse error** — `times` was read as multiplication | ✅ works |
| `size of X` | lexed to a token no parser rule handled | ✅ works |
| 175 reserved words in `.EP` | display keywords leaked into logic files | ✅ 110, see §4 |

---

## 1. Named parameters

```epl
describe divide as "Divides a by b safely"
to divide taking a, b
    check that b is not 0
    return a divided by b
!

set result by divide with 10 and 2
show result                              -- 5
```

- `taking` introduces a comma-separated parameter list.
- Parameters are scoped to the activity body.
- Arity is checked at call time.
- No limit on parameter count (v1.8 capped at 6).
- Inline one-liners work too — the comma before the body is unambiguous
  because a parameter list only continues when another name follows:

```epl
to square taking n, return n times n!
set s by square with 7
show s                                   -- 49
```

### Calling convention

`set VAR by ACTIVITY with ARG and ARG` — arguments are separated by
`with`, `and`, or `from`.

> **`by` vs `as`:** `set x by greet` calls the activity. `set x as greet`
> reads `greet` as a value — which still *works* for zero-argument
> activities, but is not the call form. Prefer `by` for calls.

### Breaking change

The implicit `x, y, z, a, b, c` binding is **removed**, not deprecated.

```epl
-- v1.8
to divide
    return x divided by y
!

-- v1.9
to divide taking a, b
    return a divided by b
!
```

Pre-launch was the cheapest possible moment to break this, and keeping both
forms would have taught every LLM reading the docs two conventions for one
concept.

---

## 2. Real error handling

### 2.1 Errors that actually raise

These all failed silently before v1.9 and now raise catchable errors:

| Operation | Raises on |
|-----------|-----------|
| `read file` | open failed / unreadable |
| `write to file`, `append to file` | open or write failed |
| `delete file` | unlink failed |
| `run` | non-zero exit status |
| `send json ... to socket` | socket create / connect / read failure |
| `post json ... to` | malformed URL, `https://` (no TLS), DNS failure, connect failure, non-2xx status |
| `field ... from json` | malformed JSON, missing key |
| `parse json` | malformed input |
| activity call | unknown name, arity mismatch |
| `check that` | assertion false |
| division | division by zero |

```epl
attempt
    set config as read file "~/app/config.json"
if fails then
    show "Could not load config: " plus error message
!
```

`error message` returns the message of the most recent error:

```
Could not load config: cannot read file '/home/tathya/app/config.json': No such file or directory
```

### 2.2 Raising your own

```epl
to withdraw taking balance, amount
    if amount is greater than balance then
        fail with "insufficient funds"
    !
    return balance minus amount
!

attempt
    set r by withdraw with 50 and 100
if fails then
    show "caught: " plus error message      -- caught: insufficient funds
!
```

### 2.3 Caught errors are quiet

Inside `attempt`, errors do not print to stderr — the handler decides what to
report. Uncaught errors print as before and stop execution.

---

## 3. Native JSON

`json.c` is a real recursive-descent parser with a native `VAL_MAP` type.
Objects become maps, arrays become lists, and values keep their types.

```epl
set doc as parse json "{\"person\":{\"name\":\"tathya\",\"tier\":2},\"tags\":[\"a\",\"b\"],\"ok\":true}"

show field "person.name" from doc     -- tathya
show field "tags.0"      from doc     -- a
show field "ok"          from doc     -- true
show to json doc                      -- round-trips, key order preserved
```

- **Dotted paths** — `field "user.name" from doc`, including array indices
  (`items.0.title`). A literal key containing dots still wins over splitting.
- **Chaining** still works: extract a sub-object, then read from it.
- **`to json X`** serializes any value back to compact JSON, with correct
  escaping. This replaces hand-splicing JSON strings:

```epl
-- v1.8: fragile and injection-prone
set body as "{\"model\":\"llama3.2:1b\",\"prompt\":\"" plus query plus "\"}"

-- v1.9
set request as parse json "{\"model\":\"llama3.2:1b\",\"stream\":false}"
set response as post json (to json request) to "http://localhost:11434/api/generate"
```

- **Backward compatible:** `field "k" from json <raw string>` still works —
  it parses on the spot, so v1.8 scripts keep running but stop getting
  corrupted results on nested input.
- Handles escapes, `\uXXXX` including surrogate pairs, and nesting to depth 128.

---

## 4. Scoped keywords

`.EP` and `.EPF` now have separate vocabularies, selected by file extension.
Display keywords (charts, layout, 3D, HTML) no longer leak into logic files.

```epl
set status as "active"      -- was a parse error before v1.9
set type   as "admin"
set value  as 42
set items  as [1, 2]
set user   as "ada"
set input  as "raw"
```

**58 words freed in `.EP` files**, including `status`, `type`, `value`, `id`,
`class`, `line`, `row`, `section`, `page`, `table`, `label`, `node`, `at`,
`x`, `y`, `z`, `image`, `button`, `star`, `core`, `radius`, `speed`.

Ten more were removed entirely as dead tokens — words reserved for parser
rules that never existed: `items`, `respectively`, `edge`, `reply`, `serve`,
`post`, `get`, `env`, `exists`, `fetch`. (`reply with`, `serve on socket`,
`post json`, `get env` and `file exists` are matched as phrases and are
unaffected.)

`user` and `input` were reserved solely as optional filler in the `provide`
statement; that statement now matches them by text instead.

---

## 5. Grammar fixes

- **`repeat N times with i`** — the README documented this but it was a parse
  error, because `times` is both a loop keyword and a multiplication operator
  and the expression parser swallowed it. The counter is **0-based**, matching
  `item N of list`:

```epl
set names as "a,b,c" split by ","
repeat size of names times with i
    show item i of names
!
```

- **`size of X`** — the lexer produced `TOKEN_SIZE_OF` but no parser rule ever
  consumed it. Now wired to the length operation.

- **`image`** was ambiguous between two EPF generations —
  `image "url"` (v1.4 gallery) and `image src "url"` (v1.7 HTML). The
  dispatcher always chose gallery, so the HTML form failed. Now resolved by
  one-token lookahead.

---

## 6. Tooling

- **`tests/`** — 21 regression cases with a runner (`./tests/run.sh`, or
  `make test`). Every defect in §0 is a test case. There was no automated test
  suite before v1.9, which is why these regressions shipped.
- **Makefile header dependencies** (`-MMD -MP`). Previously `%.o: %.c` had no
  header dependencies, so editing a shared enum in `ast.h` left stale object
  files linked against mismatched layouts — producing a binary that ran and
  silently did nothing.
- **Banner moved to stderr** so `epl script.EP > out.txt` no longer writes
  ASCII art into the output file.

---

## 7. Not in this release

Deliberately deferred so v1.9 stayed reviewable:

- **`import` / module system** → v1.10. `TOKEN_IMPORT` and `NODE_IMPORT` exist
  as stubs with no parser or interpreter support. Building a module system on
  top of positional `x,y,z` globals was not worth doing twice — which is
  exactly why §1 came first.
- **`.EPDB` data layer** → v1.10. Currently accepted as a file extension and
  run through the ordinary `.EP` pipeline. Scheduled for v1.6, then v1.7; now
  targeted at v1.10 with a minimal `make table` / `insert` / `query where`.
- **Package convention / `epl install`** → v1.10.
- **Async `post json ... in background`** → post-launch.

---

## Changelog

| Version | Focus | Key Addition |
|---------|-------|-------------|
| v1.8 | System | File I/O, shell, sockets, HTTP, JSON, IPC daemon |
| **v1.9** | **Integrity** | **Named parameters, real errors, JSON values, scoped keywords, test suite** |
| v1.10 | Ecosystem | `import`, `.EPDB` data layer, package convention, starter libraries |
