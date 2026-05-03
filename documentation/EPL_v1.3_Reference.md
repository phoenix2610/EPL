# EPL — English Programming Language
## Complete Language Reference  `v1.3`

> *"Your thoughts are the syntax."*
> EPL is an interpreted language where English is not just readable — it is the source code.
> Built for humans first, executed by a C interpreter second.
> Designed with LLMs as first-class consumers of code — not just writers of it.

---

## Table of Contents

1. [Philosophy](#1-philosophy)
2. [Running EPL](#2-running-epl)
3. [Data Types](#3-data-types)
4. [Variables — set and update](#4-variables--set-and-update)
5. [Operators](#5-operators)
6. [Comparators](#6-comparators)
7. [Logical Operators](#7-logical-operators)
8. [Showing Output](#8-showing-output)
9. [User Input — provide](#9-user-input--provide)
10. [Conditionals — if / otherwise](#10-conditionals--if--otherwise)
11. [Loops](#11-loops)
12. [Activities — defining functions](#12-activities--defining-functions)
13. [Error Handling — attempt / if fails](#13-error-handling--attempt--if-fails)
14. [Lists](#14-lists)
15. [Smart Collections *(v1.1)*](#15-smart-collections-v11)
16. [Smart Filter — where *(v1.1)*](#16-smart-filter--where-v11)
17. [String Operations *(v1.2)*](#17-string-operations-v12)
18. [Number Formatting *(v1.2)*](#18-number-formatting-v12)
19. [Type Checking *(v1.2)*](#19-type-checking-v12)
20. [Connection Type — Graph *(v1.3)*](#20-connection-type--graph-v13)
21. [Assertions — check](#21-assertions--check)
22. [Documentation — describe](#22-documentation--describe)
23. [System — execute / wait](#23-system--execute--wait)
24. [Keyword Reference](#24-keyword-reference)
25. [Operator Precedence](#25-operator-precedence)
26. [Reserved Words](#26-reserved-words)
27. [Full Examples](#27-full-examples)
28. [Benchmark Notes](#28-benchmark-notes)
29. [Roadmap](#29-roadmap)

---

## 1. Philosophy

EPL exists to solve the **vibe coder problem**.

When developers use AI to generate all their code, they lose the mental model. When the codebase grows and needs changes, they face a wall of logic they never understood. They trade speed for effectiveness — and pay the cost later.

EPL fixes the bridge. English is the language of context, and in EPL, that context *becomes* the syntax. The programmer's thought process is the source code. The LLM and the interpreter are just execution engines.

```
Human thought → EPL → C Interpreter → Execution
                ↑
         You always stay here.
         You always understand what you wrote.
```

### The describe navigation system

In a 10,000 line codebase, an LLM reading Python needs to consume the entire file — roughly 12,000+ tokens — just to understand what the program does. EPL changes this fundamentally:

```
Python:   LLM reads 10,000 lines  →  ~12,000 tokens minimum
EPL:      LLM reads 150 describe blocks  →  ~800 tokens
          → navigates directly to relevant 300 lines
          → reads those lines  →  ~400 tokens
          Total: ~1,200 tokens vs 12,000
```

The `describe` block is not a comment. It is a **semantic index** — a structured, extractable, LLM-navigable summary layer that sits on top of the code. A future `epl --describe program.EP` command produces a `.EPD` file — a pure intent summary an LLM can read to understand the entire program without ever touching the source.

### Token density vs line count

EPL's advantage is not just fewer lines. It is **semantic token density** — more meaning per token consumed. Python has noise: `def`, `self`, `__init__`, `:`, type hints, decorators. EPL has none of that. Every token is signal. This makes EPL 5-6x more efficient for LLM context consumption even if file sizes were equal.

### Full stack vision

EPL is on a path to become a full-stack language — logic, data, frontend, and relationships all expressible in English. The same `describe`-first philosophy that works for functions extends naturally to database tables, UI layouts, and system diagrams. One language. One mental model. No context switching.

---

## 2. Running EPL

### Building from source

```bash
gcc -Wall -std=c11 -o epl main.c lexer.c token.c ast.c parser.c interpreter.c repl.c -lm
```

### Running a `.EP` file

```powershell
.\epl.exe program.EP
```

### Launching the REPL (interactive mode)

```powershell
.\epl.exe
```

### Debug flags

```powershell
.\epl.exe --tokens program.EP    # show token stream
.\epl.exe --ast    program.EP    # show abstract syntax tree
```

### Fix Windows terminal encoding (for banner)

```powershell
chcp 65001
```

---

## 3. Data Types

EPL has seven built-in types. Types are inferred — never declared.

| Type     | Example values                     | Notes                               |
|----------|------------------------------------|-------------------------------------|
| Number   | `42`, `3.14`, `-7`, `0`            | All numbers are 64-bit floats       |
| String   | `"hello"`, `"EPL v1.3"`           | Double-quoted                       |
| Boolean  | `true`, `false`                    | Lowercase only                      |
| Null     | `null`, `empty`                    | Both mean "no value"                |
| List     | `[1, 2, 3]`, `["a", "b"]`         | Mixed types allowed                 |
| Activity | result of `to name ...!`           | First-class, stored as values       |
| Graph    | result of `set x as graph`         | *(v1.3)* Directed graph with BFS    |

---

## 4. Variables — `set` and `update`

### `set` — first assignment

```epl
set name as "Phoenix"
set age as 21
set running as true
set scores as [85, 92, 78, 95]
set nothing as null
set network as graph
```

### `update` — reassignment / mutation

```epl
update age as 22
update running as false
update name as "Tathya"
```

### Multi-word variable names

EPL supports multi-word variable names naturally:

```epl
set high achievers as scores where score greater than 85
set pass average as average of passed
set org chart as graph
set clean name as name trimmed
```

**Rule:** Use `set` the first time a variable appears. Use `update` for every change after that. This distinction makes the first appearance of every variable visually obvious — which is the whole point.

---

## 5. Operators

### Arithmetic

| EPL syntax          | Meaning          | Example                          |
|---------------------|------------------|----------------------------------|
| `x plus y`          | Addition         | `set r as x plus y`             |
| `x minus y`         | Subtraction      | `set r as total minus tax`      |
| `x times y`         | Multiplication   | `set area as length times width` |
| `x divided by y`    | Division         | `set avg as total divided by n`  |
| `x modulo y`        | Remainder        | `set rem as 10 modulo 3`        |
| `x power of y`      | Exponentiation   | `set sq as x power of 2`        |

### String concatenation

Use `+` to join strings and values:

```epl
show "Hello, " + name
show "Score: " + score + " out of 100"
```

**Important:** Concatenation binds left-to-right. To concatenate the *result* of arithmetic, store it first:

```epl
set result as x plus y
show "Sum: " + result        # correct

show "Sum: " + x plus y      # reads as ("Sum: " + x) plus y — wrong
```

---

## 6. Comparators

| EPL syntax             | Meaning                  |
|------------------------|--------------------------|
| `x equals y`           | Equals                   |
| `x is y`               | Equals (shorthand)       |
| `x is not y`           | Not equal                |
| `x greater than y`     | Greater than             |
| `x less than y`        | Less than                |
| `x at least y`         | Greater than or equal    |
| `x at most y`          | Less than or equal       |
| `x is any of a, b, c`  | x matches one of a/b/c  |
| `x is none of a, b`    | x matches none of a/b   |
| `x in list`            | x is in the list         |

---

## 7. Logical Operators

| EPL syntax    | Meaning   |
|---------------|-----------|
| `a and b`     | Both true |
| `a or b`      | Either    |
| `not a`       | Negation  |

```epl
if age at least 18 and has id then
    show "Entry allowed"
!

if score less than 0 or score greater than 100 then
    show "Invalid score"
!
```

---

## 8. Showing Output

```epl
show "Hello, World!"
show name
show "Result: " + result
show score
show true
show [1, 2, 3]
show network
```

### Inline conditional output

```epl
show "Pass" if score at least 60 otherwise "Fail"
show "Adult" if age at least 18 otherwise "Minor"
```

---

## 9. User Input — `provide`

```epl
provide name as user entry from "What is your name? "
provide age as user entry from "Enter your age: "
```

EPL automatically detects if the input is a number or a string — no casting needed.

---

## 10. Conditionals — `if` / `otherwise`

### Basic if

```epl
if score at least 60 then
    show "You passed!"
!
```

### If / otherwise

```epl
if temperature greater than 30 then
    show "It's hot"
otherwise
    show "It's fine"
!
```

### Chained conditions

```epl
if grade at least 90 then
    show "A"
otherwise if grade at least 80 then
    show "B"
otherwise if grade at least 70 then
    show "C"
otherwise
    show "Below C"
!
```

### Inline (one-liner)

```epl
show "Pass" if score at least 60 otherwise "Fail"
set label as "hot" if temp greater than 30 otherwise "cool"
```

---

## 11. Loops

### While loop

```epl
set count as 0
while count less than 5
    show count
    update count as count plus 1
!
```

### Repeat N times

```epl
repeat 3 times
    show "Hello!"
!
```

### Repeat N times with counter

```epl
repeat 5 times with i
    show "Step " + i
!
```

### Repeat indefinitely (with break)

```epl
repeat indefinitely
    provide choice as user entry from "Continue? (yes/no) "
    if choice is "no" then
        break
    !
!
```

### Repeat until condition

```epl
set x as 1
repeat until x greater than 10
    update x as x times 2
!
show x
```

### For each — iterate a list

```epl
set fruits as ["apple", "banana", "cherry"]
for each fruit in fruits
    show fruit
!
```

### For range — numeric range

```epl
for i from 1 to 10
    show i
!
```

### Loop control

```epl
continue    # skip to next iteration
break       # exit the loop
```

---

## 12. Activities — defining functions

Activities are EPL's functions. Parameters are auto-detected from usage inside the body.

### Defining

```epl
to greet
    show "Hello from EPL!"
!

to add
    return x plus y
!

to square
    return x power of 2
!

to divide
    check that y is not 0
    if y equals 0 then
        return "Error: Division by zero"
    !
    return x divided by y
!
```

### Calling

```epl
greet
set result as add
set sq as square
```

### Calling with `by`

```epl
set result by adding n1 and n2
set area by squaring side
```

### Activities with preconditions

```epl
to safe divide
    check that y is not 0
    return x divided by y
!
```

### Activities are first-class values

```epl
set my operation as add
```

---

## 13. Error Handling — `attempt` / `if fails`

```epl
attempt
    set result by dividing n1 by n2
if fails then
    show "Division failed"
!
```

```epl
attempt
    set data as load file "data.txt"
if fails then
    show "Could not load file"
    set data as []
!
```

---

## 14. Lists

### Creating

```epl
set scores as [85, 92, 78, 95, 67]
set names as ["Alice", "Bob", "Carol"]
set mixed as [1, "hello", true, null]
set empty list as []
```

### Adding to a list

```epl
add 100 to scores
add "Dave" to names
```

### Iterating

```epl
for each score in scores
    show score
!
```

### Checking membership

```epl
if "Alice" in names then
    show "Alice is here"
!
```

### is any of (multiple match)

```epl
if choice is any of "1", "2", "3" then
    show "Valid choice"
!
```

---

## 15. Smart Collections *(v1.1)*

Every list in EPL is **self-aware**. No imports, no loops, no boilerplate.

### Aggregation

```epl
set prices as [10, 25, 8, 42, 17, 33]

show total of prices       # 135
show average of prices     # 22.5
show maximum of prices     # 42
show minimum of prices     # 8
show length of prices      # 6
```

### Sorting

```epl
set asc as sorted prices
show asc                   # [8, 10, 17, 25, 33, 42]

set desc as reversed asc
show desc                  # [42, 33, 25, 17, 10, 8]
```

### Inline (no intermediate variable needed)

```epl
show "Highest: " + maximum of prices
show "Items: " + length of prices
show average of sorted prices
```

### Length of strings

```epl
set name as "EPL is awesome"
show length of name        # 14
```

### Chaining operations

```epl
set scores as [45, 82, 67, 91, 55, 78, 38, 95]
set top as sorted scores
show "Top score: " + maximum of top
show "Average: " + average of scores
```

### Python comparison

```python
# Python
total   = sum(prices)
average = sum(prices) / len(prices)
maximum = max(prices)
minimum = min(prices)
count   = len(prices)
asc     = sorted(prices)
desc    = list(reversed(asc))
```

```epl
# EPL v1.1
show total of prices
show average of prices
show maximum of prices
show minimum of prices
show length of prices
set asc as sorted prices
set desc as reversed asc
```

---

## 16. Smart Filter — `where` *(v1.1)*

### Syntax

```epl
set <result> as <collection> where <item> <condition>
```

### Examples

```epl
set scores as [45, 82, 67, 91, 55, 78, 38, 95]

set passed as scores where score at least 60
set failed as scores where score less than 60
set high achievers as scores where score greater than 85

show passed          # [82, 67, 91, 78, 95]
show failed          # [45, 55, 38]
show high achievers  # [91, 95]
```

### Filter then aggregate

```epl
set pass total as total of passed
set pass avg as average of passed
set pass count as length of passed

show "Passed: " + pass count + " students"
show "Their average: " + pass avg
```

### Combined in one line

```epl
show length of scores where score at least 60
show average of scores where score greater than 50
```

### Python comparison

```python
# Python
passed      = [s for s in scores if s >= 60]
high_achiev = [s for s in scores if s > 85]
pass_avg    = sum(passed) / len(passed)
```

```epl
# EPL v1.1
set passed as scores where score at least 60
set high achievers as scores where score greater than 85
set pass avg as average of passed
```

---

## 17. String Operations *(v1.2)*

### Case conversion

```epl
set name as "phoenix developer"
set upper as name in uppercase     # "PHOENIX DEVELOPER"
set lower as name in lowercase     # "phoenix developer"
```

### Trimming whitespace

```epl
set padded as "  hello world  "
set clean as padded trimmed        # "hello world"
```

### Split and join

```epl
set sentence as "the quick brown fox"
set words as sentence split by " "
show words                          # [the, quick, brown, fox]

set joined as words joined by "-"
show joined                         # the-quick-brown-fox
```

### Replace

```epl
set text as "hello world hello"
set fixed as text with "hello" replaced by "EPL"
show fixed                          # EPL world EPL
```

### Search operations — return boolean

```epl
set name as "Phoenix Developer"

show name starts with "Phoenix"     # true
show name ends with "Developer"     # true
show name contains "nix"            # true
```

### Practical example

```epl
set raw name as "  alice johnson  "
set clean name as raw name trimmed
set display name as clean name in uppercase
set parts as clean name split by " "
set first name as parts joined by ""    # with smarter split, first element

show "Display: " + display name
```

---

## 18. Number Formatting *(v1.2)*

### Currency

```epl
set price as 19.99
set salary as 75000

show price as currency       # $19.99
show salary as currency      # $75000.00
```

### Percentage

```epl
set ratio as 87.5
show ratio as percentage     # 87.5%
```

### Rounding

```epl
set pi as 3.14159265
show pi rounded to 2         # 3.14
show pi rounded to 4         # 3.1416
show pi rounded to 0         # 3
```

### Thousands formatting

```epl
set big as 1000000
show big as formatted        # 1,000,000

set salary as 75000
show salary as formatted     # 75,000
```

### Combining

```epl
set revenue as 2450000
set growth as 12.7

show "Revenue: " + revenue as formatted
show "Growth:  " + growth as percentage
```

---

## 19. Type Checking *(v1.2)*

```epl
set age as 25
set name as "Tathya"
set items as [1, 2, 3]
set flag as true
set nothing as null

if age is a number then
    show "age is a number"
!

if name is a string then
    show "name is a string"
!

if items is a list then
    show "items is a list"
!

if flag is a boolean then
    show "flag is a boolean"
!

if nothing is a number then
    show "WRONG"
otherwise
    show "nothing is not a number — correct"
!
```

### In activity preconditions

```epl
to process score
    check that score is a number
    check that score at least 0
    check that score at most 100
    return score as percentage
!
```

---

## 20. Connection Type — Graph *(v1.3)*

EPL v1.3 introduces the **graph** as a first-class data type. Graphs store directed relationships between named nodes and support shortest-path traversal via BFS.

### Creating a graph

```epl
set network as graph
set org chart as graph
```

Each variable is an independent, isolated graph. Linking nodes in `network` has no effect on `org chart`.

### Linking nodes

```epl
link customer to store in network
link store to warehouse in network
link warehouse to supplier in network
link customer to support in network
link support to manager in network
```

`link` creates a **directed edge** — `customer → store` does not imply `store → customer`. Both nodes are created automatically if they don't exist yet.

### Shortest path — `route from X to Y`

```epl
set path as route from customer to supplier in network
show path
# [customer, store, warehouse, supplier]
```

Uses **Breadth-First Search** — guaranteed shortest path in terms of number of hops. Returns a list of node names from start to end inclusive. Returns an empty list if no path exists.

```epl
set no path as route from supplier to customer in network
show no path        # [] — directed graph, no backwards route
```

### Reachability — `connected to X`

```epl
set reachable as connected to customer in network
show reachable
# [store, support, warehouse, manager, supplier]
```

Returns all nodes reachable from the given node via BFS traversal. The start node itself is excluded from the result.

### Multiple independent graphs

```epl
set supply chain as graph
link factory to distributor in supply chain
link distributor to retailer in supply chain

set org chart as graph
link ceo to cto in org chart
link ceo to cfo in org chart
link cto to engineer in org chart

set supply path as route from factory to retailer in supply chain
set org path as route from ceo to engineer in org chart

show supply path    # [factory, distributor, retailer]
show org path       # [ceo, cto, engineer]
```

### Graph + Smart Collections

```epl
set network as graph
link a to b in network
link b to c in network
link b to d in network
link c to e in network

set reachable as connected to a in network
show "Reachable nodes: " + length of reachable
show "All nodes: " + reachable
```

### Showing graph info

```epl
show network        # [graph: 6 nodes, 5 edges]
```

### Real world example — supply chain

```epl
describe supply chain as "Models a product supply chain and finds delivery routes"

set chain as graph

link factory to regional hub in chain
link regional hub to city warehouse in chain
link city warehouse to local store in chain
link factory to express depot in chain
link express depot to local store in chain

show "=== Supply Chain Analysis ==="

set standard route as route from factory to local store in chain
show "Standard route: " + standard route

set express route as route from factory to local store in chain
show "Express route:  " + express route

set reachable from factory as connected to factory in chain
show "All destinations: " + reachable from factory
show "Total stops: " + length of reachable from factory
```

### Python comparison

```python
# Python — requires a library (networkx) or 80+ lines of manual BFS
import networkx as nx
G = nx.DiGraph()
G.add_edge("customer", "store")
G.add_edge("store", "warehouse")
G.add_edge("warehouse", "supplier")
path = nx.shortest_path(G, "customer", "supplier")
reachable = list(nx.descendants(G, "customer"))
```

```epl
# EPL v1.3 — zero imports, zero library knowledge
set network as graph
link customer to store in network
link store to warehouse in network
link warehouse to supplier in network
set path as route from customer to supplier in network
set reachable as connected to customer in network
```

---

## 21. Assertions — `check`

```epl
check that age is greater than 0
check that name is not empty
check that y is not 0
check that items is not empty
check that score is a number
```

Used at the start of activities as preconditions:

```epl
to calculate bmi
    check that weight is greater than 0
    check that height is greater than 0
    return weight divided by (height power of 2)
!
```

---

## 22. Documentation — `describe`

`describe` is EPL's self-documenting layer and its most strategically important feature.

```epl
describe calculator as "A simple CLI calculator with history"

describe add as "Returns the sum of x and y"
to add
    return x plus y
!

describe divide as "Divides x by y — checks for zero before dividing"
to divide
    check that y is not 0
    if y equals 0 then
        return "Error: Division by zero"
    !
    return x divided by y
!
```

### Why describe is not a comment

Comments are ignored by compilers, skipped by developers, and invisible to LLMs unless the LLM reads the whole file. `describe` blocks are **structured, parseable, and extractable**. They survive the tokenizer and parser as first-class AST nodes.

### The describe navigation system

In a 10,000 line EPL program with 150 describe blocks, an LLM can:

1. Read all 150 describe blocks (~800 tokens)
2. Identify the relevant section (`"Employee system, lines 100-400"`)
3. Read only those 300 lines (~400 tokens)
4. Fix the bug, add the feature, answer the question

**Total: ~1,200 tokens.** A Python equivalent of 5,000 lines would require the LLM to consume the entire file — 5,000–8,000 tokens — just to find the relevant section. EPL is **5-6x more efficient for LLM-assisted development** even at identical file sizes.

### Future: `.EPD` semantic index

```powershell
epl --describe program.EP    # produces program.EPD
```

The `.EPD` file contains only describe blocks and activity signatures. An LLM reading `.EPD` understands the entire program's intent without reading a single line of logic. This is the checkpoint system — restore LLM context on any codebase in under 1,000 tokens.

---

## 23. System — `execute` / `wait`

```epl
execute system command "cls" if windows otherwise "clear"
wait for enter key with message "Press Enter to continue..."
```

---

## 24. Keyword Reference

### Control flow

| Keyword        | Purpose                              |
|----------------|--------------------------------------|
| `if`           | Conditional branch                   |
| `then`         | Opens an if block                    |
| `otherwise`    | Else branch                          |
| `while`        | Condition-based loop                 |
| `repeat`       | Count-based or indefinite loop       |
| `times`        | Used with repeat N                   |
| `until`        | Loop until condition is true         |
| `indefinitely` | Loop forever (use break to exit)     |
| `for each`     | Iterate a list                       |
| `for`          | Range loop                           |
| `from`         | Range start / route start            |
| `to`           | Range end / link target              |
| `in`           | Membership / iteration / graph name  |
| `with`         | Loop counter / string replace        |
| `break`        | Exit current loop                    |
| `continue`     | Skip to next iteration               |

### Variables

| Keyword    | Purpose                         |
|------------|---------------------------------|
| `set`      | First assignment of a variable  |
| `update`   | Reassignment / mutation         |
| `as`       | Assignment connector            |
| `by`       | Activity call connector         |

### Activities

| Keyword     | Purpose                              |
|-------------|--------------------------------------|
| `to`        | Define an activity                   |
| `!`         | End of block                         |
| `return`    | Return a value from an activity      |

### Types and values

| Keyword     | Purpose                    |
|-------------|----------------------------|
| `true`      | Boolean true               |
| `false`     | Boolean false              |
| `null`      | No value                   |
| `empty`     | Alias for null             |
| `graph`     | *(v1.3)* Create a graph    |

### Input / Output

| Keyword      | Purpose                         |
|--------------|---------------------------------|
| `show`       | Print to terminal               |
| `provide`    | Read user input                 |
| `user entry` | Input source specifier          |

### Smart Collections *(v1.1)*

| Keyword    | Purpose                           |
|------------|-----------------------------------|
| `average`  | Mean of a numeric list            |
| `total`    | Sum of a numeric list             |
| `maximum`  | Highest value in a list           |
| `minimum`  | Lowest value in a list            |
| `length`   | Count of items (list or string)   |
| `sorted`   | Ascending copy of a list          |
| `reversed` | Reversed copy of a list           |
| `of`       | Connector for collection ops      |

### Smart Filter *(v1.1)*

| Keyword  | Purpose                                  |
|----------|------------------------------------------|
| `where`  | Filter a list by a per-item condition    |

### String Operations *(v1.2)*

| Keyword      | Purpose                              |
|--------------|--------------------------------------|
| `uppercase`  | Convert string to uppercase          |
| `lowercase`  | Convert string to lowercase          |
| `trimmed`    | Remove leading/trailing whitespace   |
| `split`      | Split string into list by delimiter  |
| `joined`     | Join list into string with delimiter |
| `replaced`   | Replace substring                    |
| `starts`     | Check string prefix                  |
| `ends`       | Check string suffix                  |
| `contains`   | Check substring presence             |

### Number Formatting *(v1.2)*

| Keyword      | Purpose                              |
|--------------|--------------------------------------|
| `currency`   | Format as $X.XX                      |
| `percentage` | Format as X%                         |
| `rounded`    | Round to N decimal places            |
| `formatted`  | Add thousands separators             |

### Type Checking *(v1.2)*

| Keyword    | Purpose                      |
|------------|------------------------------|
| `a`        | Type check connector         |
| `number`   | Number type keyword          |
| `string`   | String type keyword          |
| `boolean`  | Boolean type keyword         |
| `list`     | List type keyword            |

### Connection Type *(v1.3)*

| Keyword     | Purpose                                     |
|-------------|---------------------------------------------|
| `graph`     | Create a new graph value                    |
| `link`      | Add a directed edge between two nodes       |
| `route`     | BFS shortest path between two nodes         |
| `connected` | All nodes reachable from a given node       |

### Other

| Keyword     | Purpose                                   |
|-------------|-------------------------------------------|
| `add`       | Add item to a list                        |
| `check`     | Assert a condition (runtime check)        |
| `describe`  | Document an activity or module            |
| `attempt`   | Try a block — catch errors               |
| `if fails`  | Handler for attempt failures              |
| `execute`   | Run a system command                      |
| `wait`      | Pause for user key press                  |
| `is any of` | Multi-value match                         |
| `is none of`| Negative multi-value match                |
| `not`       | Logical negation                          |
| `and`       | Logical AND                               |
| `or`        | Logical OR                                |

---

## 25. Operator Precedence

Highest to lowest:

| Level | Operators                                         |
|-------|---------------------------------------------------|
| 1     | `not` (unary)                                     |
| 2     | `power of`                                        |
| 3     | `times`, `divided by`, `modulo`                   |
| 4     | `plus`, `minus`                                   |
| 5     | `+` (string concat)                               |
| 6     | `greater than`, `less than`, `at least`, `at most`|
| 7     | `equals`, `is`, `is not`, `in`, `is any of`, `is none of`, `is a` |
| 8     | `and`                                             |
| 9     | `or`                                              |
| 10    | `if ... otherwise` (inline conditional)           |
| 11    | `where` (filter)                                  |
| 12    | Postfix: `in uppercase`, `trimmed`, `split by`, `as currency`, `rounded to` |

---

## 26. Reserved Words

```
a          add        and        as         at         attempt
average    boolean    break      by         check      connected
contains   continue   currency   describe   divided    each
edge       else       empty      ends       execute    fails
false      for        formatted  from       given      graph
if         in         indefinitely is       joined     length
less       link       list       lowercase  maximum    minimum
modulo     node       not        null       number     of
or         otherwise  percentage plus       power      provide
repeat     replaced   return     reversed   route      rounded
set        show       sorted     split      starts     string
then       times      to         total      trimmed    true
until      update     uppercase  user       wait       where
while      with
```

---

## 27. Full Examples

### 27.1 EPL Calculator

```epl
describe calculator as "A simple CLI calculator with history, built in EPL v1.3"

set history as []
set running as true

describe add op as "Returns sum of x and y"
to add op
    return x plus y
!

describe subtract op as "Returns x minus y"
to subtract op
    return x minus y
!

describe multiply op as "Returns x times y"
to multiply op
    return x times y
!

describe divide op as "Divides x by y, checks for zero"
to divide op
    check that y is not 0
    if y equals 0 then
        return "Error: Division by zero"
    !
    return x divided by y
!

to clear screen
    execute system command "cls" if windows otherwise "clear"
!

to show menu
    show "--- EPL Calculator v1.3 ---"
    show "1. Add (+)"
    show "2. Subtract (-)"
    show "3. Multiply (*)"
    show "4. Divide (/)"
    show "5. View History"
    show "6. Exit"
!

while running
    show menu
    provide choice as user entry from "Select an option (1-6): "

    if choice is "6" then
        show "Goodbye!"
        update running as false
        continue
    !

    if choice is "5" then
        clear screen
        show "--- Calculation History ---"
        show "No records found." if history is empty otherwise ""
        for each rec in history
            show rec
        !
        wait for enter key with message "Press Enter to return..."
        continue
    !

    if choice is any of "1", "2", "3", "4" then
        provide n1 as user entry from "Enter first number: "
        provide n2 as user entry from "Enter second number: "

        set result as 0
        set op as "?"

        if choice is "1" then
            set result as n1 plus n2
            update op as "+"
        !
        if choice is "2" then
            set result as n1 minus n2
            update op as "-"
        !
        if choice is "3" then
            set result as n1 times n2
            update op as "*"
        !
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

### 27.2 Student Grades Analysis *(v1.1 + v1.2)*

```epl
describe grade analyzer as "Analyzes student scores using Smart Collections, Filter, and Formatting"

set scores as [45, 82, 67, 91, 55, 78, 38, 95, 73, 61]
set student name as "  alice johnson  "

show "=== " + student name trimmed in uppercase + " ==="
show "Total students: " + length of scores
show "Highest score:  " + maximum of scores
show "Lowest score:   " + minimum of scores
show "Class average:  " + average of scores rounded to 1
show "Total marks:    " + total of scores as formatted

show ""
show "=== Pass / Fail Analysis ==="
set passed as scores where score at least 60
set failed as scores where score less than 60
set distinction as scores where score at least 85

show "Passed:      " + length of passed + " students"
show "Failed:      " + length of failed + " students"
show "Distinction: " + length of distinction + " students"
show "Pass rate:   " + (length of passed times 100 divided by length of scores) as percentage

show ""
show "=== Passed Group Stats ==="
show "Average: " + average of passed rounded to 1
show "Highest: " + maximum of passed
show "Lowest:  " + minimum of passed
show "Scores:  " + sorted passed
```

---

### 27.3 Supply Chain Graph *(v1.3)*

```epl
describe supply chain as "Models a product supply chain with routing and reachability"

set chain as graph

link factory to regional hub in chain
link regional hub to city warehouse in chain
link city warehouse to local store in chain
link factory to express depot in chain
link express depot to local store in chain
link regional hub to outlet in chain

show "=== Supply Chain ==="
show chain

show ""
show "=== Routes ==="
set standard as route from factory to local store in chain
set to outlet as route from factory to outlet in chain
show "Factory → Local Store: " + standard
show "Factory → Outlet:      " + to outlet

show ""
show "=== Reachability ==="
set all destinations as connected to factory in chain
show "All reachable from factory: " + all destinations
show "Total destinations: " + length of all destinations
```

---

### 27.4 Organisation Hierarchy *(v1.3)*

```epl
describe org as "Maps reporting structure and finds management chains"

set org chart as graph

link ceo to cto in org chart
link ceo to cfo in org chart
link ceo to coo in org chart
link cto to engineering lead in org chart
link cto to security lead in org chart
link engineering lead to senior engineer in org chart
link cfo to finance lead in org chart

show "=== Org Chart ==="
show org chart

show ""
show "=== Reporting Chains ==="
set path to engineer as route from ceo to senior engineer in org chart
set path to finance as route from ceo to finance lead in org chart
show "CEO → Senior Engineer: " + path to engineer
show "CEO → Finance Lead:    " + path to finance

show ""
show "=== Under CEO (all reports) ==="
set all reports as connected to ceo in org chart
show all reports
show "Total headcount under CEO: " + length of all reports
```

---

### 27.5 Number Guessing Game

```epl
describe guessing game as "Player guesses a secret number between 1 and 100"

set secret as 42
set attempts as 0

show "Guess the number between 1 and 100!"

repeat indefinitely
    provide guess as user entry from "Your guess: "
    update attempts as attempts plus 1

    if guess equals secret then
        show "Correct! You got it in " + attempts + " attempts."
        break
    !

    if guess less than secret then
        show "Too low! Try higher."
    otherwise
        show "Too high! Try lower."
    !
!
```

---

## 28. Benchmark Notes

| Metric               | Python        | EPL v1.3        | Notes                              |
|----------------------|---------------|-----------------|------------------------------------|
| Lines of code        | ~147          | ~116            | EPL 21% shorter                    |
| Token count          | ~1850         | ~1420           | EPL 23% fewer tokens               |
| LLM readability      | Needs training| Zero prior knowledge | EPL wins by design             |
| Wall time (startup)  | ~45 ms        | ~52 ms          | Both dominated by startup          |
| Pure logic time      | ~0.38 ms      | ~0.05 ms        | EPL wins on raw execution          |
| Gap cause            | —             | +7ms parse overhead | Fixable with `.EPC` cache      |
| Graph ops            | Requires networkx | Built-in    | EPL wins — zero imports            |
| LLM context (10k LOC)| ~12,000 tokens| ~1,200 tokens   | EPL 10x more efficient via describe|

**Upcoming optimization:** `.EPC` compiled cache — parse once, cache bytecode, skip re-parsing on subsequent runs. Projected EPL startup: `< 5ms`.

---

## 29. Roadmap

### ✅ v1.0 — Core Language
- `set` / `update`, arithmetic, comparators, logical operators
- `if` / `otherwise` conditionals
- All loop variants — `while`, `repeat`, `for each`, `for range`
- `to ... !` activity definitions + `by` calling
- `attempt` / `if fails` error handling
- `provide` user input, `check` assertions
- `describe` documentation blocks
- `add to` list mutation, `execute` / `wait` system calls
- REPL + `.EP` file execution

### ✅ v1.1 — Smart Collections + Smart Filter
- `average`, `total`, `maximum`, `minimum`, `length` — list aggregation
- `sorted`, `reversed` — list ordering
- `where` clause — declarative list filtering
- Filter-then-aggregate chaining

### ✅ v1.2 — Richer Types
- String ops: `in uppercase`, `in lowercase`, `trimmed`, `split by`, `joined by`, `with X replaced by Y`, `starts with`, `ends with`, `contains`
- Number formatting: `as currency`, `as percentage`, `rounded to N`, `as formatted`
- Type checking: `is a number`, `is a string`, `is a list`, `is a boolean`

### ✅ v1.3 — Connection Type
- `set X as graph` — per-variable directed graph
- `link X to Y in graph` — directed edge creation
- `route from X to Y in graph` — BFS shortest path
- `connected to X in graph` — BFS reachability
- Graph + Smart Collections chaining
- Multiple independent graphs in one program

### 🔲 v1.4 — Output / Visualization
- `make line chart for x axis as ... and y axis as ...`
- `make bar chart`, `make table`, `make report`
- Terminal and HTML output targets

### 🔲 v1.5 — `.EPD` Semantic Index
- `epl --describe program.EP` → `program.EPD`
- Describe-block extraction and structured export
- LLM context checkpoint system
- Section-range tagging (`describe` knows its line range)

### 🔲 v1.6 — Data Layer
- `make table users` — relational table definition
- `query users where age greater than 18`
- `insert into users`, `delete from users where`
- Full SQL-equivalent in English

### 🔲 v1.7 — Frontend Layer
- `make page`, `section`, `heading`, `button`, `input field`
- HTML-equivalent output
- `links to`, `on click`, `style as`

### 🔲 v2.0 — Full Stack Runtime + LLM-Native
- Single `.EP` file compiles to HTML + data layer + logic
- `remember` / `forget` — persistent session context
- `.EPC` compiled bytecode cache — sub-5ms startup
- EPL as a complete full-stack language

---

*EPL is built on the belief that if you can describe what you want in English, you should be able to run it.*
*v1.3 — May 2026*
