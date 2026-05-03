# EPL — English Programming Language
## Complete Language Reference  `v1.4`

> *"Your thoughts are the syntax."*
> EPL is an interpreted language where English is not just readable — it is the source code.
> Built for humans first, executed by a C interpreter second.
> Designed with LLMs as first-class consumers of code — not just writers of it.

---

## Table of Contents

1. [Philosophy](#1-philosophy)
2. [File Types](#2-file-types)
3. [Running EPL](#3-running-epl)
4. [Data Types](#4-data-types)
5. [Variables — set and update](#5-variables--set-and-update)
6. [Operators](#6-operators)
7. [Comparators](#7-comparators)
8. [Logical Operators](#8-logical-operators)
9. [Showing Output](#9-showing-output)
10. [User Input — provide](#10-user-input--provide)
11. [Conditionals — if / otherwise](#11-conditionals--if--otherwise)
12. [Loops](#12-loops)
13. [Activities — defining functions](#13-activities--defining-functions)
14. [Error Handling — attempt / if fails](#14-error-handling--attempt--if-fails)
15. [Lists](#15-lists)
16. [Smart Collections *(v1.1)*](#16-smart-collections-v11)
17. [Smart Filter — where *(v1.1)*](#17-smart-filter--where-v11)
18. [String Operations *(v1.2)*](#18-string-operations-v12)
19. [Number Formatting *(v1.2)*](#19-number-formatting-v12)
20. [Type Checking *(v1.2)*](#20-type-checking-v12)
21. [Connection Type — Graph *(v1.3)*](#21-connection-type--graph-v13)
22. [Frontend Pages — EPF *(v1.4)*](#22-frontend-pages--epf-v14)
23. [Assertions — check](#23-assertions--check)
24. [Documentation — describe](#24-documentation--describe)
25. [System — execute / wait](#25-system--execute--wait)
26. [Keyword Reference](#26-keyword-reference)
27. [Operator Precedence](#27-operator-precedence)
28. [Reserved Words](#28-reserved-words)
29. [Full Examples](#29-full-examples)
30. [Benchmark Notes](#30-benchmark-notes)
31. [Roadmap](#31-roadmap)

---

## 1. Philosophy

EPL exists to solve the **vibe coder problem**.

When developers use AI to generate all their code, they lose the mental model. When the codebase grows and needs changes, they face a wall of logic they never understood. They trade speed for effectiveness — and pay the cost later.

EPL fixes the bridge. English is the language of context, and in EPL, that context *becomes* the syntax. The programmer's thought process is the source code.

```
Human thought → EPL → C Interpreter → Execution
                ↑
         You always stay here.
         You always understand what you wrote.
```

### The describe navigation system

In a 10,000 line codebase, an LLM reading Python needs to consume the entire file — roughly 12,000+ tokens. EPL changes this fundamentally:

```
Python:   LLM reads 10,000 lines  →  ~12,000 tokens minimum
EPL:      LLM reads 150 describe blocks  →  ~800 tokens
          → navigates to relevant 300 lines  →  ~400 tokens
          Total: ~1,200 tokens  vs  12,000
```

The `describe` block is not a comment. It is a **semantic index** — structured, extractable, LLM-navigable. A future `epl --describe program.EP` produces a `.EPD` file — pure intent, no logic — that an LLM reads to understand an entire program in under 1,000 tokens.

### Full stack vision

EPL is on a path to become a full-stack language. Logic, data, and frontend — all expressible in English. One language. One mental model. No context switching.

---

## 2. File Types

EPL uses three distinct file extensions. Same English syntax across all three. Same keywords, same operators, same mental model. Each file type activates only the interpreter modules it needs.

| Extension | Purpose | Produces |
|-----------|---------|---------|
| `.EP`     | Logic, computation, backend | Execution output, terminal |
| `.EPF`    | Frontend pages, charts, tables | `.html` files (live-bound) |
| `.EPDB`   | Data definitions, queries, persistence *(v1.6)* | `.epdb` store |

### How they connect

```epf
# dashboard.EPF
import data from "storage.EPDB"
import logic from "program.EP"

make page saved as "dashboard.html" styled as "dark"
    heading "Sales Dashboard"
    make line chart for x axis as months and y axis as revenue titled "Revenue"
!
```

The `import` keyword — arriving in v1.6 — lets all three file types share data. `.EP` computes. `.EPDB` persists. `.EPF` presents. Each does one thing, in English.

### Live binding — `.epbind`

Every time an `.EPF` file generates an HTML output, EPL writes a `.epbind` file next to the source:

```json
{
  "outputs": [
    { "file": "dashboard.html", "type": "html", "updated": "2026-05-03 14:22:11" }
  ]
}
```

The `.EP` / `.EPF` file is the **source of truth**. The HTML is a compiled artifact — EPL owns it. Re-run the `.EPF` → HTML regenerates with updated data. Add a new `make chart` → new entry appears in `.epbind`. The connection is tracked and live.

---

## 3. Running EPL

### Building from source

```bash
gcc -Wall -std=c11 -o epl main.c lexer.c token.c ast.c parser.c interpreter.c repl.c codegen.c binding.c -lm
```

### Running files

```powershell
.\epl.exe program.EP          # logic execution
.\epl.exe dashboard.EPF       # generates dashboard.html
.\epl.exe storage.EPDB        # data operations (v1.6)
```

### REPL (interactive mode)

```powershell
.\epl.exe
```

### Debug flags

```powershell
.\epl.exe --tokens program.EP
.\epl.exe --ast    program.EP
```

### Fix Windows terminal encoding

```powershell
chcp 65001
```

---

## 4. Data Types

EPL has seven built-in types. Types are inferred — never declared.

| Type     | Example values              | Notes                            |
|----------|-----------------------------|----------------------------------|
| Number   | `42`, `3.14`, `-7`          | All numbers are 64-bit floats    |
| String   | `"hello"`, `"EPL v1.4"`    | Double-quoted                    |
| Boolean  | `true`, `false`             | Lowercase only                   |
| Null     | `null`, `empty`             | Both mean "no value"             |
| List     | `[1, 2, 3]`, `["a", "b"]`  | Mixed types allowed              |
| Activity | result of `to name ...!`    | First-class, stored as values    |
| Graph    | result of `set x as graph`  | *(v1.3)* Directed graph with BFS |

---

## 5. Variables — `set` and `update`

```epl
set name as "Phoenix"
set age as 21
set scores as [85, 92, 78, 95]
set network as graph
```

```epl
update age as 22
update name as "Tathya"
```

### Multi-word variable names

```epl
set high achievers as scores where score greater than 85
set pass average as average of passed
set org chart as graph
set clean name as name trimmed
```

**Rule:** `set` on first appearance. `update` for every change after.

---

## 6. Operators

| EPL syntax          | Meaning        |
|---------------------|----------------|
| `x plus y`          | Addition       |
| `x minus y`         | Subtraction    |
| `x times y`         | Multiplication |
| `x divided by y`    | Division       |
| `x modulo y`        | Remainder      |
| `x power of y`      | Exponentiation |
| `"a" + "b"`         | Concatenation  |

**Concatenation note:** Store arithmetic results before concatenating:
```epl
set result as x plus y
show "Sum: " + result       # correct
```

---

## 7. Comparators

| EPL syntax             | Meaning               |
|------------------------|-----------------------|
| `x equals y`           | Equals                |
| `x is y`               | Equals (shorthand)    |
| `x is not y`           | Not equal             |
| `x greater than y`     | Greater than          |
| `x less than y`        | Less than             |
| `x at least y`         | Greater than or equal |
| `x at most y`          | Less than or equal    |
| `x is any of a, b, c`  | Matches one of        |
| `x is none of a, b`    | Matches none of       |
| `x in list`            | Membership            |

---

## 8. Logical Operators

```epl
if age at least 18 and has id then
    show "Entry allowed"
!

if score less than 0 or score greater than 100 then
    show "Invalid score"
!
```

| Syntax    | Meaning   |
|-----------|-----------|
| `a and b` | Both true |
| `a or b`  | Either    |
| `not a`   | Negation  |

---

## 9. Showing Output

```epl
show "Hello, World!"
show name
show "Score: " + score
show [1, 2, 3]
show "Pass" if score at least 60 otherwise "Fail"
```

---

## 10. User Input — `provide`

```epl
provide name as user entry from "What is your name? "
provide age as user entry from "Enter your age: "
```

EPL auto-detects number or string — no casting needed.

---

## 11. Conditionals — `if` / `otherwise`

```epl
if score at least 60 then
    show "You passed!"
!

if grade at least 90 then
    show "A"
otherwise if grade at least 80 then
    show "B"
otherwise
    show "Below B"
!

show "Pass" if score at least 60 otherwise "Fail"
```

---

## 12. Loops

```epl
# While
while count less than 5
    update count as count plus 1
!

# Repeat N times
repeat 3 times
    show "Hello!"
!

# Repeat with counter
repeat 5 times with i
    show "Step " + i
!

# Repeat indefinitely
repeat indefinitely
    if done then break !
!

# Repeat until
repeat until x greater than 100
    update x as x times 2
!

# For each
for each fruit in fruits
    show fruit
!

# For range
for i from 1 to 10
    show i
!
```

---

## 13. Activities — defining functions

```epl
to add
    return x plus y
!

to divide
    check that y is not 0
    return x divided by y
!
```

```epl
set result as add
set area by squaring side
```

---

## 14. Error Handling — `attempt` / `if fails`

```epl
attempt
    set result as n1 divided by n2
if fails then
    show "Division failed"
!
```

---

## 15. Lists

```epl
set scores as [85, 92, 78, 95, 67]
add 100 to scores

for each score in scores
    show score
!

if "Alice" in names then
    show "Alice is here"
!
```

---

## 16. Smart Collections *(v1.1)*

```epl
set prices as [10, 25, 8, 42, 17, 33]

show total of prices       # 135
show average of prices     # 22.5
show maximum of prices     # 42
show minimum of prices     # 8
show length of prices      # 6

set asc as sorted prices
set desc as reversed asc
```

Works inline too:
```epl
show "Highest: " + maximum of prices
show average of sorted prices
```

`length of` also works on strings:
```epl
show length of "EPL is awesome"   # 14
```

---

## 17. Smart Filter — `where` *(v1.1)*

```epl
set passed as scores where score at least 60
set failed as scores where score less than 60
set top as scores where score greater than 85

show average of passed
show length of failed
show total of passed
```

---

## 18. String Operations *(v1.2)*

```epl
set name as "  Phoenix Developer  "

show name in uppercase          # "  PHOENIX DEVELOPER  "
show name in lowercase          # "  phoenix developer  "
show name trimmed               # "Phoenix Developer"

set words as sentence split by " "
set joined as words joined by "-"

set fixed as text with "hello" replaced by "EPL"

show name starts with "  Ph"    # true
show name ends with "er  "      # true
show name contains "Developer"  # true
```

---

## 19. Number Formatting *(v1.2)*

```epl
show price as currency          # $19.99
show salary as currency         # $75000.00
show ratio as percentage        # 87.5%
show pi rounded to 2            # 3.14
show big as formatted           # 1,000,000
```

---

## 20. Type Checking *(v1.2)*

```epl
if age is a number then
    show "age is a number"
!

if name is a string then
    show "name is a string"
!

if items is a list then
    show "items is a list"
!
```

---

## 21. Connection Type — Graph *(v1.3)*

```epl
set network as graph

link customer to store in network
link store to warehouse in network
link warehouse to supplier in network

set path as route from customer to supplier in network
show path        # [customer, store, warehouse, supplier]

set reachable as connected to customer in network
show reachable   # [store, warehouse, supplier]
```

### Multiple independent graphs

```epl
set org chart as graph
link ceo to cto in org chart
link cto to engineer in org chart

set supply as graph
link factory to distributor in supply
link distributor to retailer in supply
```

`org chart` and `supply` are completely isolated. Linking in one never affects the other.

### Why graphs in EPL vs Python

```python
# Python — requires networkx or 80+ lines of manual BFS
import networkx as nx
G = nx.DiGraph()
G.add_edge("customer", "store")
path = nx.shortest_path(G, "customer", "supplier")
```

```epl
# EPL — zero imports, zero library knowledge
set network as graph
link customer to store in network
set path as route from customer to supplier in network
```

---

## 22. Frontend Pages — EPF *(v1.4)*

`.EPF` files generate self-contained HTML pages — Chart.js embedded inline, full CSS embedded, zero external dependencies. Open anywhere. No internet needed.

### Basic structure

```epf
make page saved as "dashboard.html" styled as "dark"

    heading "My Dashboard"
    subheading "Subtitle text here"
    paragraph "Some body text explaining what this shows."
    divider

    section "Charts"
        make line chart for x axis as months and y axis as revenue titled "Revenue"
    !

    section "Data"
        make table from students titled "Students" with headers ["Name", "Score"]
    !

!
```

### Text elements

| EPF syntax              | Output         |
|-------------------------|----------------|
| `heading "text"`        | `<h1>`         |
| `subheading "text"`     | `<h2>`         |
| `label "text"`          | `<h4>`         |
| `paragraph "text"`      | `<p>`          |
| `bold "text"`           | `<strong>`     |
| `divider`               | `<hr>`         |
| `spacer`                | vertical gap   |

Text elements accept any EPL expression — not just string literals:

```epf
set title as "Report for " + student name in uppercase
heading title
paragraph "Average score: " + average of scores rounded to 1
```

### Layout

```epf
# Full-width stacked (default)
section "Stats"
    make stat "Total" from total of revenue
    make stat "Average" from average of scores
!

# Side by side
row
    make stat "Total" from total of revenue
    make stat "Average" from average of scores
    make stat "Maximum" from maximum of scores
!
```

`row` uses CSS grid — auto-fits as many items as fit at minimum 200px each. No CSS knowledge needed.

### Stat cards

```epf
make stat "Average Score" from average of scores
make stat "Pass Rate" from pass rate as percentage
make stat "Total Revenue" from total of revenue as formatted
make stat "Top Score" from maximum of scores
```

Large bold number, labelled underneath. The value can be any EPL expression including formatting:

```epf
make stat "Revenue" from 2450000 as formatted       # 2,450,000
make stat "Growth"  from 12.7 as percentage         # 12.7%
make stat "Price"   from 99.99 as currency          # $99.99
```

### Charts

```epf
# Line chart
set months as ["Jan", "Feb", "Mar", "Apr", "May", "Jun"]
set revenue as [12000, 19000, 15000, 25000, 22000, 31000]
make line chart for x axis as months and y axis as revenue titled "Monthly Revenue"

# Bar chart
set subjects as ["Math", "Science", "English", "Art"]
set scores as [85, 92, 78, 91]
make bar chart for x axis as subjects and y axis as scores titled "Subject Scores"

# Pie chart
set categories as ["Passed", "Failed", "Distinction"]
set values as [65, 20, 15]
make pie chart from categories and values titled "Grade Distribution"
```

Charts use data from EPL variables — the same lists used in computation. No data duplication.

### Tables

```epf
set students as [["Alice", 91], ["Bob", 85], ["Carol", 78]]
make table from students titled "Student List" with headers ["Name", "Score"]

# Single-column (no headers)
set names as ["Alice", "Bob", "Carol"]
make table from names titled "Attendees"
```

### Reports

```epf
make report from scores titled "Class Statistics"
```

Auto-generates: Count, Total, Average, Maximum, Minimum — for any numeric list. No configuration needed.

### Themes

EPF supports five named themes plus unlimited custom colors:

```epf
styled as "light"       # default — clean white
styled as "dark"        # dark slate
styled as "indigo"      # purple tones
styled as "emerald"     # green tones
styled as "rose"        # red/pink tones
```

### Custom colors — zero new syntax *(v1.4 addition)*

Any unrecognised style string is treated as a custom accent color. EPL auto-derives a complete light palette from that single color. Named colors, multi-word names, and hex codes all work natively:

```epf
# Friendly multi-word names (lookup table)
styled as "persian blue"     # → #1c39bb
styled as "hot pink"         # → hotpink
styled as "midnight blue"    # → midnightblue
styled as "forest green"     # → forestgreen
styled as "amber"            # → #ffbf00
styled as "charcoal"         # → #36454f

# Raw hex codes
styled as "#ff69b4"
styled as "#1c39bb"
styled as "#2ecc71"

# Single-word CSS color names
styled as "coral"
styled as "teal"
styled as "orchid"
styled as "crimson"
```

**How it works:** `THEME_CUSTOM` is the enum value for any unknown string. `codegen_get_theme` derives a full consistent palette — background, surface, text, borders — using the custom color as the accent. The lookup table (`static const`, ~2KB in read-only data, ~35 entries) maps friendly multi-word names to valid CSS values. Linear search — negligible at parse time.

**Implementation cost:** Zero new keywords. Zero new tokens. Zero new AST nodes. One enum value added to `EPFTheme`. Two small edits — parser stores unknown style strings in `color[]` and sets `THEME_CUSTOM`, codegen handles the new case.

### with color (explicit accent override)

For overriding just the accent on a named theme:

```epf
make page saved as "report.html" styled as "dark" with color "#f59e0b"
```

### Sections

```epf
section "Revenue Trends"
    make line chart for x axis as months and y axis as revenue titled "Monthly"
    make bar chart for x axis as quarters and y axis as targets titled "Targets"
!

section "Student Data"
    make table from students titled "Records" with headers ["Name", "Score", "Grade"]
    make report from scores titled "Statistics"
!
```

Sections render as rounded cards with a subtle border. Title appears as an `h3`.

### Variables in EPF

EPL logic works fully inside `.EPF` files — `set`, `update`, `where`, Smart Collections, all of it:

```epf
set scores as [45, 82, 67, 91, 55, 78, 38, 95]
set passed as scores where score at least 60
set avg as average of scores
set pass rate as length of passed times 100 divided by length of scores

make page saved as "report.html" styled as "indigo"

    heading "Grade Report"

    row
        make stat "Average" from avg rounded to 1
        make stat "Pass Rate" from pass rate as percentage
        make stat "Top Score" from maximum of scores
        make stat "Passed" from length of passed
    !

    section "Score Distribution"
        make bar chart for x axis as grades and y axis as counts titled "Distribution"
    !

!
```

### Python + HTML comparison

```python
# Python — to generate this dashboard you need:
# matplotlib or plotly for charts
# pandas for data processing
# jinja2 or flask for HTML templating
# knowledge of Chart.js or D3.js
# CSS for styling
# ~200 lines minimum

import matplotlib.pyplot as plt
import pandas as pd
# ... 200 lines ...
```

```epf
# EPF — zero imports, zero library knowledge, ~30 lines

set months as ["Jan", "Feb", "Mar"]
set revenue as [12000, 19000, 25000]

make page saved as "dashboard.html" styled as "dark"
    heading "Revenue Dashboard"
    make line chart for x axis as months and y axis as revenue titled "Revenue"
    make stat "Total" from total of revenue as formatted
!
```

---

## 23. Assertions — `check`

```epl
check that age is greater than 0
check that name is not empty
check that y is not 0
check that score is a number
```

---

## 24. Documentation — `describe`

```epl
describe calculator as "CLI calculator with history"

describe add as "Returns sum of x and y"
to add
    return x plus y
!
```

### Why describe beats comments

Comments are ignored by compilers and skipped by developers. `describe` blocks are first-class AST nodes — structured, parseable, extractable. An LLM reading 150 describe blocks understands a 10,000 line program in ~800 tokens. That's the semantic index.

### Future: `.EPD` checkpoint file

```powershell
epl --describe program.EP    # produces program.EPD
```

The `.EPD` contains only describe blocks and activity signatures. An LLM reads it to understand the entire program's intent — no logic required. Restore full LLM context on any codebase in under 1,000 tokens.

---

## 25. System — `execute` / `wait`

```epl
execute system command "cls" if windows otherwise "clear"
wait for enter key with message "Press Enter to continue..."
```

---

## 26. Keyword Reference

### Control flow

| Keyword        | Purpose                          |
|----------------|----------------------------------|
| `if`           | Conditional branch               |
| `then`         | Opens if block                   |
| `otherwise`    | Else branch                      |
| `while`        | Condition-based loop             |
| `repeat`       | Count or indefinite loop         |
| `times`        | Used with repeat N               |
| `until`        | Loop until condition             |
| `indefinitely` | Loop forever                     |
| `for each`     | Iterate a list                   |
| `for`          | Range loop                       |
| `from`         | Range start / route start        |
| `to`           | Range end / link target          |
| `in`           | Membership / iteration / graph   |
| `with`         | Counter / replace / headers      |
| `break`        | Exit loop                        |
| `continue`     | Skip to next iteration           |

### Variables

| Keyword  | Purpose                        |
|----------|--------------------------------|
| `set`    | First assignment               |
| `update` | Reassignment                   |
| `as`     | Assignment connector           |
| `by`     | Activity call connector        |

### Activities

| Keyword  | Purpose                   |
|----------|---------------------------|
| `to`     | Define an activity        |
| `!`      | End of block              |
| `return` | Return from activity      |

### Types

| Keyword   | Purpose          |
|-----------|------------------|
| `true`    | Boolean true     |
| `false`   | Boolean false    |
| `null`    | No value         |
| `empty`   | Alias for null   |
| `graph`   | Create graph     |

### Smart Collections *(v1.1)*

| Keyword    | Purpose                         |
|------------|---------------------------------|
| `average`  | Mean of list                    |
| `total`    | Sum of list                     |
| `maximum`  | Highest value                   |
| `minimum`  | Lowest value                    |
| `length`   | Count (list or string)          |
| `sorted`   | Ascending copy                  |
| `reversed` | Reversed copy                   |
| `of`       | Collection op connector         |
| `where`    | Filter by condition             |

### String Operations *(v1.2)*

| Keyword      | Purpose                       |
|--------------|-------------------------------|
| `uppercase`  | Convert to uppercase          |
| `lowercase`  | Convert to lowercase          |
| `trimmed`    | Remove whitespace             |
| `split`      | Split to list                 |
| `joined`     | Join list to string           |
| `replaced`   | Replace substring             |
| `starts`     | Prefix check                  |
| `ends`       | Suffix check                  |
| `contains`   | Substring check               |

### Number Formatting *(v1.2)*

| Keyword      | Purpose                       |
|--------------|-------------------------------|
| `currency`   | Format as $X.XX               |
| `percentage` | Format as X%                  |
| `rounded`    | Round to N decimal places     |
| `formatted`  | Add thousands separators      |

### Type Checking *(v1.2)*

| Keyword   | Purpose                  |
|-----------|--------------------------|
| `a`       | Type check connector     |
| `number`  | Number type              |
| `string`  | String type              |
| `boolean` | Boolean type             |
| `list`    | List type                |

### Connection Type *(v1.3)*

| Keyword     | Purpose                          |
|-------------|----------------------------------|
| `graph`     | Create graph                     |
| `link`      | Add directed edge                |
| `route`     | BFS shortest path                |
| `connected` | All reachable nodes              |

### Frontend — EPF *(v1.4)*

| Keyword      | Purpose                          |
|--------------|----------------------------------|
| `page`       | Create HTML page                 |
| `section`    | Grouped card                     |
| `row`        | Side-by-side layout              |
| `heading`    | h1 text                          |
| `subheading` | h2 text                          |
| `paragraph`  | p text                           |
| `label`      | h4 text                          |
| `bold`       | strong text                      |
| `divider`    | Horizontal rule                  |
| `spacer`     | Vertical gap                     |
| `stat`       | Big metric card                  |
| `line`       | Line chart type                  |
| `bar`        | Bar chart type                   |
| `pie`        | Pie chart type                   |
| `chart`      | Chart keyword                    |
| `table`      | Data table                       |
| `report`     | Auto-stats block                 |
| `titled`     | Title for chart/table/report     |
| `saved`      | Output filename                  |
| `styled`     | Theme name or custom color       |
| `headers`    | Column header list               |
| `x`          | X axis specifier                 |
| `y`          | Y axis specifier                 |
| `axis`       | Axis connector                   |
| `color`      | Explicit accent override         |
| `import`     | Import from another EPL file     |

### Other

| Keyword     | Purpose                    |
|-------------|----------------------------|
| `add`       | Add item to list           |
| `check`     | Assert condition           |
| `describe`  | Document activity/module   |
| `attempt`   | Try block                  |
| `if fails`  | Error handler              |
| `execute`   | System command             |
| `wait`      | Pause for keypress         |
| `is any of` | Multi-value match          |
| `is none of`| Negative multi-match       |
| `not`       | Logical negation           |
| `and`       | Logical AND                |
| `or`        | Logical OR                 |

---

## 27. Operator Precedence

| Level | Operators                                                    |
|-------|--------------------------------------------------------------|
| 1     | `not` (unary)                                                |
| 2     | `power of`                                                   |
| 3     | `times`, `divided by`, `modulo`                              |
| 4     | `plus`, `minus`                                              |
| 5     | `+` (string concat)                                          |
| 6     | `greater than`, `less than`, `at least`, `at most`           |
| 7     | `equals`, `is`, `is not`, `in`, `is any of`, `is none of`, `is a` |
| 8     | `and`                                                        |
| 9     | `or`                                                         |
| 10    | `if ... otherwise` (inline conditional)                      |
| 11    | `where` (filter)                                             |
| 12    | Postfix: `in uppercase`, `trimmed`, `as currency`, `rounded to` |

---

## 28. Reserved Words

```
a          add        and        as         at         attempt
average    axis       bar        boolean    bold       break
by         chart      check      color      connected  contains
continue   currency   describe   divider    divided    each
edge       else       empty      ends       execute    fails
false      for        formatted  from       graph      heading
headers    if         import     in         indefinitely is
joined     label      length     less       line       link
list       lowercase  maximum    minimum    modulo     node
not        null       number     of         or         otherwise
page       paragraph  percentage pie        plus       power
provide    repeat     replaced   report     return     reversed
route      rounded    row        saved      section    set
show       sorted     spacer     split      starts     stat
string     styled     subheading table      then       times
titled     to         total      trimmed    true       until
update     uppercase  user       wait       where      while
with       x          y
```

---

## 29. Full Examples

### 29.1 EPL Calculator

```epl
describe calculator as "CLI calculator with history — EPL v1.4"

set history as []
set running as true

to add op
    return x plus y
!
to subtract op
    return x minus y
!
to multiply op
    return x times y
!
to divide op
    check that y is not 0
    if y equals 0 then return "Error: Division by zero" !
    return x divided by y
!

to show menu
    show "--- EPL Calculator ---"
    show "1. Add  2. Subtract  3. Multiply  4. Divide  5. History  6. Exit"
!

while running
    show menu
    provide choice as user entry from "Select: "

    if choice is "6" then
        show "Goodbye!"
        update running as false
        continue
    !
    if choice is "5" then
        show "No records found." if history is empty otherwise ""
        for each rec in history
            show rec
        !
        continue
    !
    if choice is any of "1", "2", "3", "4" then
        provide n1 as user entry from "First number: "
        provide n2 as user entry from "Second number: "
        set result as 0
        set op as "?"
        if choice is "1" then set result as n1 plus n2  update op as "+" !
        if choice is "2" then set result as n1 minus n2 update op as "-" !
        if choice is "3" then set result as n1 times n2 update op as "*" !
        if choice is "4" then
            attempt
                set result as n1 divided by n2
            if fails then
                update result as "Error: Division by zero"
            !
            update op as "/"
        !
        set record as n1 + " " + op + " " + n2 + " = " + result
        show "Result: " + record
        add record to history
    otherwise
        show "Invalid selection."
    !
!
```

---

### 29.2 Grade Analysis + Dashboard

**grades.EP** — computation:
```epl
describe grade analyzer as "Analyzes scores, formats results for dashboard"

set scores as [45, 82, 67, 91, 55, 78, 38, 95, 73, 61]
set subjects as ["Math", "Science", "English", "History", "Art"]
set subject scores as [85, 92, 78, 91, 74]

set passed as scores where score at least 60
set failed as scores where score less than 60
set distinction as scores where score at least 85
set pass rate as length of passed times 100 divided by length of scores
```

**dashboard.EPF** — presentation:
```epf
describe dashboard as "Grade dashboard — Term 2 2026"

set scores as [45, 82, 67, 91, 55, 78, 38, 95, 73, 61]
set subjects as ["Math", "Science", "English", "History", "Art"]
set subject scores as [85, 92, 78, 91, 74]
set months as ["Jan", "Feb", "Mar", "Apr", "May"]
set revenue as [12000, 19000, 15000, 25000, 22000]
set students as [["Alice", 91], ["Bob", 85], ["Carol", 78], ["Dave", 92], ["Eve", 55]]

set passed as scores where score at least 60
set pass rate as length of passed times 100 divided by length of scores

make page saved as "dashboard.html" styled as "dark"

    heading "Grade Dashboard"
    subheading "Term 2 — Academic Year 2026"
    paragraph "Comprehensive performance summary for the current term."
    divider

    section "Key Metrics"
        row
            make stat "Average Score"    from average of scores rounded to 1
            make stat "Top Score"        from maximum of scores
            make stat "Pass Rate"        from pass rate as percentage
            make stat "Distinctions"     from length of distinction
        !
    !

    section "Performance by Subject"
        make bar chart for x axis as subjects and y axis as subject scores titled "Subject Scores"
    !

    section "Score Distribution"
        set grade labels as ["Distinction", "Pass", "Fail"]
        set grade counts as [length of distinction, length of passed minus length of distinction, length of failed]
        make pie chart from grade labels and grade counts titled "Grade Breakdown"
    !

    section "Student Records"
        make table from students titled "Student List" with headers ["Name", "Score"]
    !

    section "Statistics"
        make report from scores titled "Class Statistics"
    !

!
```

---

### 29.3 Supply Chain Graph + Report

```epf
describe supply report as "Supply chain analysis with routing and reachability"

set chain as graph
link factory to regional hub in chain
link regional hub to city warehouse in chain
link city warehouse to local store in chain
link factory to express depot in chain
link express depot to local store in chain

set standard as route from factory to local store in chain
set all stops as connected to factory in chain

make page saved as "supply.html" styled as "persian blue"

    heading "Supply Chain Report"
    subheading "Route Analysis — May 2026"
    divider

    section "Network Overview"
        row
            make stat "Total Nodes"    from length of all stops plus 1
            make stat "Route Length"   from length of standard
        !
        paragraph "Standard route: factory → regional hub → city warehouse → local store"
    !

!
```

---

## 30. Benchmark Notes

| Metric                   | Python         | EPL v1.4          | Notes                              |
|--------------------------|----------------|-------------------|------------------------------------|
| Lines of code            | ~147           | ~116              | EPL 21% shorter                    |
| Token count              | ~1850          | ~1420             | EPL 23% fewer tokens               |
| LLM readability          | Needs training | Zero prior knowledge | EPL wins by design              |
| Wall time (startup)      | ~45 ms         | ~52 ms            | Both dominated by startup          |
| Pure logic time          | ~0.38 ms       | ~0.05 ms          | EPL wins on raw execution          |
| Dashboard generation     | ~200 lines + 5 imports | ~30 lines, 0 imports | EPL wins       |
| Graph ops                | Requires networkx | Built-in       | EPL wins — zero imports            |
| LLM context (10k LOC)    | ~12,000 tokens | ~1,200 tokens     | EPL 10x via describe               |

---

## 31. Roadmap

### ✅ v1.0 — Core Language
Variables, arithmetic, conditionals, all loops, activities, error handling, input, assertions, describe, REPL, `.EP` execution.

### ✅ v1.1 — Smart Collections + Smart Filter
`average`, `total`, `maximum`, `minimum`, `length`, `sorted`, `reversed`, `where` clause, filter-then-aggregate chaining.

### ✅ v1.2 — Richer Types
String ops (uppercase, lowercase, trimmed, split, join, replace, starts/ends/contains), number formatting (currency, percentage, rounded, formatted), type checking (is a number/string/list/boolean).

### ✅ v1.3 — Connection Type
Per-variable directed graphs, `link`, BFS `route`, BFS `connected`, multiple isolated graphs, graph + Smart Collections chaining.

### ✅ v1.4 — Frontend Pages (EPF)
`.EPF` file type, `make page`, sections, rows, stat cards, line/bar/pie charts, tables, reports, five named themes, unlimited custom colors (hex, CSS names, friendly multi-word names via lookup table), live `.epbind` binding system, `codegen.c` + `binding.c` infrastructure.

### 🔲 v1.5 — `.EPD` Semantic Index
`epl --describe program.EP` → `program.EPD`, describe block extraction, section range tagging, LLM context checkpoint system.

### 🔲 v1.6 — Data Layer (EPDB)
`.EPDB` file type, EPL-native database (no SQL), `make table` definitions, `insert`, `delete`, `query where`, graph layer wired to persistence, `.epdb` binary store.

### 🔲 v1.7 — Dynamic Frontend
Forms, `input field`, `button` with `on click`, state, user interaction, live data from `.EPDB`.

### 🔲 v2.0 — Full Stack Runtime
`import` across all three file types, `.EPC` bytecode cache (sub-5ms startup), `remember`/`forget` persistent context, single `epl run` boots entire app.

---

*EPL is built on the belief that if you can describe what you want in English, you should be able to run it.*
*v1.4 — May 2026*
