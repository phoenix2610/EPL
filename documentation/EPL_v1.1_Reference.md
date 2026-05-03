# EPL — English Programming Language
## Complete Language Reference  `v1.1`

> *"Your thoughts are the syntax."*
> EPL is an interpreted language where English is not just readable — it is the source code.
> Built for humans first, executed by a C interpreter second.

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
17. [Assertions — check](#17-assertions--check)
18. [Documentation — describe](#18-documentation--describe)
19. [System — execute / wait](#19-system--execute--wait)
20. [Keyword Reference](#20-keyword-reference)
21. [Operator Precedence](#21-operator-precedence)
22. [Reserved Words](#22-reserved-words)
23. [Full Examples](#23-full-examples)
24. [Benchmark Notes](#24-benchmark-notes)
25. [Roadmap](#25-roadmap)

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

EPL is not trying to replace Python or C. It is trying to ensure that the person writing the program **always knows what their program does** — in plain English.

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

EPL has six built-in types. Types are inferred — never declared.

| Type     | Example values                     | Notes                          |
|----------|------------------------------------|--------------------------------|
| Number   | `42`, `3.14`, `-7`, `0`            | All numbers are 64-bit floats  |
| String   | `"hello"`, `"EPL v1.1"`           | Double-quoted                  |
| Boolean  | `true`, `false`                    | Lowercase only                 |
| Null     | `null`, `empty`                    | Both mean "no value"           |
| List     | `[1, 2, 3]`, `["a", "b"]`         | Mixed types allowed            |
| Activity | result of `to name ...!`           | First-class, stored as values  |

---

## 4. Variables — `set` and `update`

### `set` — first assignment

```epl
set name as "Phoenix"
set age as 21
set running as true
set scores as [85, 92, 78, 95]
set nothing as null
```

### `update` — reassignment / mutation

```epl
update age as 22
update running as false
update name as "Tathya"
```

**Rule:** Use `set` the first time a variable appears. Use `update` for every change after that.
This distinction is intentional — it makes the first appearance of every variable visually obvious.

---

## 5. Operators

### Arithmetic

| EPL syntax          | Meaning          | Example                        |
|---------------------|------------------|--------------------------------|
| `x plus y`          | Addition         | `set r as x plus y`           |
| `x minus y`         | Subtraction      | `set r as total minus tax`    |
| `x times y`         | Multiplication   | `set area as l times w`       |
| `x divided by y`    | Division         | `set avg as total divided by n`|
| `x modulo y`        | Remainder        | `set rem as 10 modulo 3`      |
| `x power of y`      | Exponentiation   | `set sq as x power of 2`      |

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

| EPL syntax           | Meaning                  |
|----------------------|--------------------------|
| `x equals y`         | Equals                   |
| `x is y`             | Equals (shorthand)       |
| `x is not y`         | Not equal                |
| `x greater than y`   | Greater than             |
| `x less than y`      | Less than                |
| `x at least y`       | Greater than or equal    |
| `x at most y`        | Less than or equal       |
| `x is any of a, b, c`| x matches one of a/b/c  |
| `x is none of a, b`  | x matches none of a/b   |
| `x in list`          | x is in the list         |

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

Every list in EPL v1.1 is **self-aware**. No imports, no loops, no boilerplate.

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

`length of` also works on strings:

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
# Python — what you used to write
total   = sum(prices)
average = sum(prices) / len(prices)
maximum = max(prices)
minimum = min(prices)
count   = len(prices)
asc     = sorted(prices)
desc    = list(reversed(asc))
```

```epl
# EPL v1.1 — what you write now
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

The `where` clause turns filtering into a conversation with your data.

### Syntax

```epl
set <result> as <collection> where <item> <condition>
```

### Examples

```epl
set scores as [45, 82, 67, 91, 55, 78, 38, 95]

# Filter by threshold
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
show "Their total: " + pass total
```

### Combined in one line

```epl
show length of scores where score at least 60
show average of scores where score greater than 50
```

### Python comparison

```python
# Python
passed       = [s for s in scores if s >= 60]
high_achiev  = [s for s in scores if s > 85]
pass_avg     = sum(passed) / len(passed)
```

```epl
# EPL v1.1
set passed as scores where score at least 60
set high achievers as scores where score greater than 85
set pass avg as average of passed
```

---

## 17. Assertions — `check`

`check` is a lightweight assertion. It validates a condition and raises a runtime error if it fails. In production builds, checks can be stripped — zero cost.

```epl
check that age is greater than 0
check that name is not empty
check that y is not 0
check that items is not empty
```

Used typically at the start of activities as preconditions:

```epl
to calculate bmi
    check that weight is greater than 0
    check that height is greater than 0
    return weight divided by (height power of 2)
!
```

---

## 18. Documentation — `describe`

`describe` is EPL's self-documenting layer. Unlike comments (which are ignored), `describe` blocks are **first-class syntax** — they survive parsing and can be extracted into a semantic summary.

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

### Why describe beats comments

Comments are ignored by everyone — the compiler, the LLM, the next developer. `describe` blocks are structured and extractable. A future EPL tool (`epl --describe program.EP`) will produce a `.EPD` summary file — a pure intent file an LLM can read to understand the entire program **without reading the code**.

```
Human thought → describe → .EPD checkpoint → LLM resumes context
```

This is the core of EPL's LLM-native design.

---

## 19. System — `execute` / `wait`

### Execute a system command

```epl
execute system command "cls" if windows otherwise "clear"
```

### Wait for user input (pause)

```epl
wait for enter key with message "Press Enter to continue..."
```

---

## 20. Keyword Reference

### Control flow

| Keyword       | Purpose                              |
|---------------|--------------------------------------|
| `if`          | Conditional branch                   |
| `then`        | Opens an if block                    |
| `otherwise`   | Else branch                          |
| `while`       | Condition-based loop                 |
| `repeat`      | Count-based or indefinite loop       |
| `times`       | Used with repeat N                   |
| `until`       | Loop until condition is true         |
| `indefinitely`| Loop forever (use break to exit)     |
| `for each`    | Iterate a list                       |
| `for`         | Range loop                           |
| `from`        | Range start                          |
| `to`          | Range end                            |
| `in`          | Membership / iteration target        |
| `with`        | Loop counter name                    |
| `break`       | Exit current loop                    |
| `continue`    | Skip to next iteration               |

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
| `!`         | End of block (activity, if, loop...) |
| `return`    | Return a value from an activity      |

### Types and values

| Keyword     | Purpose                    |
|-------------|----------------------------|
| `true`      | Boolean true               |
| `false`     | Boolean false              |
| `null`      | No value                   |
| `empty`     | Alias for null             |

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

## 21. Operator Precedence

Highest to lowest:

| Level | Operators                               |
|-------|-----------------------------------------|
| 1     | `not` (unary)                           |
| 2     | `power of`                              |
| 3     | `times`, `divided by`, `modulo`         |
| 4     | `plus`, `minus`                         |
| 5     | `+` (string concat)                     |
| 6     | `greater than`, `less than`, `at least`, `at most` |
| 7     | `equals`, `is`, `is not`, `in`, `is any of`, `is none of` |
| 8     | `and`                                   |
| 9     | `or`                                    |
| 10    | `if ... otherwise` (inline conditional) |
| 11    | `where` (filter)                        |

**Tip:** When in doubt, store intermediate results in variables. It also makes the code more readable — which is the whole point.

---

## 22. Reserved Words

These words have special meaning in EPL and cannot be used as variable names:

```
add        and        as         at         attempt    average
break      by         check      continue   describe   divided
each       else       empty      execute    fails      false
for        from       given      if         in         indefinitely
is         length     less       maximum    minimum    modulo
not        null       of         or         otherwise  plus
power      provide    repeat     return     reversed   set
show       sorted     then       times      to         total
true       until      update     user       wait       where
while      with
```

---

## 23. Full Examples

### 23.1 EPL Calculator

```epl
describe calculator as "A simple CLI calculator with history, built in EPL v1.1"

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
    show "--- EPL Calculator v1.1 ---"
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

### 23.2 Student Grades Analysis *(v1.1)*

```epl
describe grade analyzer as "Analyzes student scores using Smart Collections and Smart Filter"

set scores as [45, 82, 67, 91, 55, 78, 38, 95, 73, 61]

show "=== Grade Report ==="
show "Total students: " + length of scores
show "Highest score:  " + maximum of scores
show "Lowest score:   " + minimum of scores
show "Class average:  " + average of scores
show "Total marks:    " + total of scores

show ""
show "=== Sorted Results ==="
set ranked as sorted scores
show ranked

show ""
show "=== Pass / Fail Analysis ==="
set passed as scores where score at least 60
set failed as scores where score less than 60
set distinction as scores where score at least 85

show "Passed:      " + length of passed + " students"
show "Failed:      " + length of failed + " students"
show "Distinction: " + length of distinction + " students"

show ""
show "=== Passed Group Stats ==="
show "Average of passed: " + average of passed
show "Highest passed:    " + maximum of passed
show "Lowest passed:     " + minimum of passed
```

---

### 23.3 Fibonacci Sequence

```epl
describe fibonacci as "Generates the first N fibonacci numbers"

set a as 0
set b as 1
set count as 10

show "Fibonacci sequence:"
repeat count times
    show a
    set temp as a plus b
    update a as b
    update b as temp
!
```

---

### 23.4 Number Guessing Game

```epl
describe guessing game as "Player guesses a secret number between 1 and 100"

set secret as 42
set guessed as false
set attempts as 0

show "Guess the number between 1 and 100!"

repeat indefinitely
    provide guess as user entry from "Your guess: "
    update attempts as attempts plus 1

    if guess equals secret then
        show "Correct! You got it in " + attempts + " attempts."
        update guessed as true
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

## 24. Benchmark Notes

| Metric              | Python        | EPL v1.1        | Notes                            |
|---------------------|---------------|-----------------|----------------------------------|
| Lines of code       | ~147          | ~116            | EPL 21% shorter                  |
| Token count         | ~1850         | ~1420           | EPL 23% fewer tokens             |
| LLM readability     | Needs training| Zero prior knowledge | EPL wins by design           |
| Wall time (startup) | ~45 ms        | ~56 ms          | Both dominated by startup        |
| Pure logic time     | ~0.38 ms      | ~0.05 ms        | EPL wins on raw execution        |
| Gap cause           | —             | +10ms parse overhead | Fixable with `.EPC` cache   |

**Upcoming optimization:** `.EPC` compiled cache — parse once, run instantly on subsequent calls. Projected EPL startup: `< 5ms`.

---

## 25. Roadmap

### ✅ v1.0 — Core Language
- `set` / `update` — variable system
- All arithmetic and comparison operators
- `if` / `otherwise` — conditionals
- All loop variants — `while`, `repeat`, `for each`, `for range`
- `to ... !` — activity definitions
- `attempt` / `if fails` — error handling
- `provide` — user input
- `check` — assertions
- `describe` — documentation
- `add to` — list mutation
- `execute` / `wait` — system calls
- REPL — interactive prompt
- `.EP` file execution

### ✅ v1.1 — Smart Collections + Smart Filter
- `average of`, `total of`, `maximum of`, `minimum of` — list aggregation
- `length of` — list and string length
- `sorted`, `reversed` — list ordering
- `where` clause — declarative list filtering
- Filter-then-aggregate chaining

### 🔲 v1.2 — Richer Types
- String operations: `in uppercase`, `in lowercase`, `trimmed`, `split by`, `joined by`
- Number formatting: `as currency`, `as percentage`, `rounded to N`
- Type checking: `is a number`, `is a string`, `is a list`

### 🔲 v1.3 — Connection Type
- `link X to Y` — relationship mapping
- `route from X to Y` — graph traversal
- `connected to` — relationship queries

### 🔲 v1.4 — Output / Visualization
- `make line chart for x axis as ... and y axis as ...`
- `make table from ...`
- `make report from ...`

### 🔲 v2.0 — LLM-Native Features
- `remember` / `forget` — persistent session context
- `.EPD` describe summary file generation (`epl --describe program.EP`)
- Semantic index for LLM context restoration without re-reading full code

---

*EPL is built on the belief that if you can describe what you want in English, you should be able to run it.*
*v1.1 — May 2026*
