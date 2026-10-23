# EPL — English Programming Language
## Official Language Reference · Version 1.0

> *"Your thoughts are the syntax."*

EPL is a programming language written in plain English. It is designed for human comprehension first, LLM processing second, and machine execution third. Every `.EP` file is readable by someone who has never programmed before.

---

## Table of Contents

1. [Philosophy](#philosophy)
2. [Running EPL](#running-epl)
3. [File Structure](#file-structure)
4. [Comments](#comments)
5. [Variables](#variables)
6. [Data Types](#data-types)
7. [Operators](#operators)
8. [Control Flow](#control-flow)
9. [Loops](#loops)
10. [Activities (Functions)](#activities)
11. [Collections (Lists)](#collections)
12. [User Input & Output](#input-output)
13. [Error Handling](#error-handling)
14. [System Commands](#system-commands)
15. [Documentation Blocks](#documentation-blocks)
16. [Assertions](#assertions)
17. [Reserved Keywords](#reserved-keywords)
18. [Operator Precedence](#operator-precedence)
19. [Complete Example](#complete-example)

---

## Philosophy

EPL is built on one idea:

```
Traditional:  Human Thought → Translation Tax → Code → Machine
EPL:          Human Thought → EPL (readable receipt) → Interpreter → Machine
```

The programmer never loses the thread. EPL code IS the thought process, written down. This makes EPL uniquely suited for:

- Beginners learning programming logic without syntax barriers
- Vibe coders who need to understand what their code actually does
- LLM-assisted development where the human stays in the loop
- Teaching programming concepts in plain English

---

## Running EPL

EPL programs are stored in `.EP` files and run through the EPL interpreter.

```bash
# Run a program
./epl program.EP

# Launch interactive REPL
./epl

# Debug: view token stream
./epl --tokens program.EP

# Debug: view AST
./epl --ast program.EP
```

### REPL Commands

Once inside the REPL, the following special commands are available:

| Command  | Effect                          |
|----------|---------------------------------|
| `:help`  | Show help and examples          |
| `:env`   | Show all current variables      |
| `:clear` | Clear the terminal screen       |
| `:exit`  | Quit the REPL                   |

---

## File Structure

EPL files use the `.EP` extension. There is no required structure — statements execute top to bottom. Activities (functions) defined anywhere in the file are available everywhere.

```epl
### My first EPL program ###

set name as "World"
show "Hello, " + name
```

---

## Comments

### Single Line Comment
```epl
# This is a single line comment
set x as 10  # inline comment
```

### Multi-Line Comment
```epl
###
This is a
multi-line comment block.
Everything between the hashes is ignored.
###
```

Comments are stripped at parse time and have zero runtime cost.

---

## Variables

### Creating a Variable — `set`

`set` initializes a new variable. Use it for the first assignment only.

```epl
set name as "Alice"
set age as 30
set active as true
set score as 98.5
set items as []
```

### Updating a Variable — `update`

`update` modifies an existing variable. Using `set` on an existing name also works but `update` signals intent clearly.

```epl
set score as 0
update score as score plus 10
update score as score plus 5
```

### Naming Rules

- Variable names are one or more words
- Names are case-sensitive (`Name` and `name` are different)
- Names can contain letters, digits, and underscores
- Names cannot start with a digit
- Multi-word names are supported: `total price`, `first name`, `is running`

```epl
set total price as 0
set first name as "John"
set is running as true
```

---

## Data Types

EPL has six runtime value types.

### Number
Integer or floating point. EPL handles both transparently.

```epl
set count as 42
set price as 19.99
set pi as 3.14159
set negative as -7
```

### String
Text enclosed in double quotes. Supports escape sequences.

```epl
set greeting as "Hello, World!"
set path as "C:\\Users\\name"
set newline as "Line one\nLine two"
```

Escape sequences supported:

| Sequence | Meaning       |
|----------|---------------|
| `\n`     | Newline       |
| `\t`     | Tab           |
| `\"`     | Double quote  |
| `\\`     | Backslash     |

### Boolean
Logical true or false.

```epl
set active as true
set done as false
```

### Null
Represents the absence of a value.

```epl
set result as null
```

### List
An ordered collection of values. Lists can hold mixed types.

```epl
set numbers as [1, 2, 3, 4, 5]
set names as ["Alice", "Bob", "Charlie"]
set mixed as [42, "hello", true, null]
set empty as []
```

### Activity
A stored function definition. Assigned automatically when you define an activity.

```epl
to greet, return "Hello!"!
# 'greet' is now a variable of type activity
```

---

## Operators

### Arithmetic Operators

| EPL Syntax     | Meaning        | Example                    |
|----------------|----------------|----------------------------|
| `plus`         | Addition       | `x plus y`                 |
| `minus`        | Subtraction    | `x minus y`                |
| `times`        | Multiplication | `x times y`                |
| `divided by`   | Division       | `x divided by y`           |
| `modulo`       | Remainder      | `x modulo y`               |
| `power of`     | Exponentiation | `x power of 2`             |
| `+`            | Concatenation  | `"Hello " + name`          |

### Comparison Operators

| EPL Syntax       | Meaning               | Example                         |
|------------------|-----------------------|---------------------------------|
| `is`             | Equality              | `choice is "1"`                 |
| `equals`         | Equality              | `y equals 0`                    |
| `is not`         | Inequality            | `name is not ""`                |
| `not equals`     | Inequality            | `x not equals y`                |
| `greater than`   | Greater than          | `age greater than 18`           |
| `less than`      | Less than             | `price less than 100`           |
| `at least`       | Greater than or equal | `score at least 60`             |
| `at most`        | Less than or equal    | `count at most 10`              |
| `is any of`      | Membership check      | `choice is any of "1","2","3"`  |
| `is none of`     | Non-membership check  | `status is none of "x","y"`     |
| `in`             | Collection membership | `5 in numbers`                  |

### Logical Operators

| EPL Syntax | Meaning     | Example                          |
|------------|-------------|----------------------------------|
| `and`      | Logical AND | `x greater than 0 and x less than 10` |
| `or`       | Logical OR  | `choice is "y" or choice is "yes"` |
| `not`      | Logical NOT | `not active`                     |

### String Concatenation

The `+` symbol concatenates strings. Numbers and booleans are automatically converted.

```epl
set msg as "Score: " + score
set info as name + " is " + age + " years old"
```

---

## Control Flow

### If Statement

```epl
if condition then
    # statements
!
```

### If — Otherwise (Else)

```epl
if condition then
    # statements when true
otherwise
    # statements when false
!
```

### Inline If (Ternary)

```epl
set status as "Pass" if score at least 60 otherwise "Fail"
show "Adult" if age at least 18 otherwise "Minor"
```

### Chained Conditions

```epl
if x greater than 0 and x less than 100 then
    show "x is between 0 and 100"
!

if name is "Alice" or name is "Bob" then
    show "Known user"
!
```

### Membership Check

```epl
if choice is any of "1", "2", "3", "4" then
    show "Valid option"
!

if status is none of "error", "failed", "unknown" then
    show "Status is clean"
!
```

---

## Loops

### While Loop

Runs while the condition is true.

```epl
set count as 0
while count less than 5
    show count
    update count as count plus 1
!
```

### Repeat N Times

```epl
repeat 10 times
    show "Hello"
!
```

### Repeat N Times With Counter

Access the current iteration index via a named counter.

```epl
repeat 5 times with i
    show "Step " + i
!
```

### Repeat Until

Runs until the condition becomes true.

```epl
set done as false
repeat until done
    show "Working..."
    update done as true
!
```

### Repeat Indefinitely

Runs forever until `break` is called.

```epl
repeat indefinitely
    provide choice as user entry from "Continue? (y/n): "
    if choice is "n" then
        break
    !
!
```

### For Each

Iterates over every item in a collection.

```epl
set fruits as ["apple", "banana", "cherry"]
for each fruit in fruits
    show "Fruit: " + fruit
!
```

### For Range

Iterates from a start value to an end value.

```epl
for i from 1 to 10
    show i
!
```

### For Range (Downward)

```epl
for i from 10 down to 1
    show i
!
```

### Loop Control

| Keyword    | Effect                              |
|------------|-------------------------------------|
| `break`    | Exit the loop immediately           |
| `continue` | Skip to the next iteration          |

```epl
for i from 1 to 10
    if i equals 5 then
        continue
    !
    if i equals 8 then
        break
    !
    show i
!
```

---

## Activities

Activities are EPL's functions. They are defined with `to`, contain a body, and return values with `return`.

### Single-Line Activity

```epl
to add, return x plus y!
to square, return x times x!
to greet, return "Hello, " + x!
```

### Multi-Line Activity

```epl
to calculate tax
    if x greater than 50000 then
        return x times 0.3
    !
    return x times 0.2
!
```

### Calling an Activity

Activities are called using `set ... by`:

```epl
# No arguments
set msg by greet

# One argument — becomes x
set result by square 4

# Two arguments — become x and y
set total by add 10 and 20

# From a source
set output by calculate tax
```

### Parameter Mapping

EPL automatically maps positional arguments to parameter names:

| Position | Parameter Name |
|----------|---------------|
| 1st arg  | `x`           |
| 2nd arg  | `y`           |
| 3rd arg  | `z`           |
| 4th arg  | `a`           |
| 5th arg  | `b`           |
| 6th arg  | `c`           |

```epl
to multiply, return x times y!

set result by multiply 6 and 7
show result   # 42
```

### Return Statement

Activities return values with `return`. Execution stops immediately at `return`.

```epl
to divide
    if y equals 0 then
        return "Error: Division by zero"
    !
    return x divided by y
!
```

---

## Collections

### Creating a List

```epl
set numbers as [10, 20, 30, 40, 50]
set names as ["Alice", "Bob", "Charlie"]
set empty as []
```

### Adding to a List

```epl
set history as []
add "first entry" to history
add 42 to history
add true to history
```

### Iterating a List

```epl
for each item in numbers
    show item
!
```

### Checking Membership

```epl
if "Alice" in names then
    show "Alice is in the list"
!
```

### Checking Empty

```epl
if history is empty then
    show "No records yet"
!
```

---

## Input & Output

### Showing Output

```epl
show "Hello, World!"
show name
show "Score: " + score
show "Result: " + x plus y   # careful: concatenation binds first
```

For computed values, store first then show:

```epl
set result as x plus y
show "Result: " + result
```

`display` is an alias for `show`:

```epl
display "Alternative output keyword"
```

### Reading User Input

```epl
provide name as user entry from "Enter your name: "
provide choice as user entry from "Select (1-6): "
provide n1 as user entry from "Enter a number: "
```

EPL automatically detects if the input is a number or string. If the user types `42`, the variable becomes a number. If they type `hello`, it becomes a string.

### Waiting for Enter

```epl
wait for enter key with message "Press Enter to continue..."
```

---

## Error Handling

### Attempt Block

Wraps risky code. If anything inside fails, execution jumps to `if fails`.

```epl
attempt
    set result as x divided by y
if fails then
    show "Something went wrong"
!
```

### Check Assertion

Validates a condition at runtime. If the check fails, EPL reports an error and stops.

```epl
check that age is greater than 0
check that name is not empty
check that price is at least 0
```

`that` is optional:

```epl
check age is greater than 0
```

---

## System Commands

### Execute a System Command

```epl
execute system command "cls"
execute system command "clear"
```

### Conditional System Command

Runs different commands based on operating system.

```epl
execute system command "cls" if windows otherwise "clear"
```

---

## Documentation Blocks

`describe` attaches a human-readable description to any activity or concept. Unlike comments, `describe` blocks are parsed and stored — they serve as semantic checkpoints for both humans and LLMs reading the code.

```epl
describe calculator as "A CLI calculator with 4 operations and history"

describe add as "Returns the sum of x and y"
to add, return x plus y!

describe divide as "Divides x by y. Returns error string on division by zero"
to divide
    if y equals 0 then
        return "Error: Division by zero"
    !
    return x divided by y
!
```

### Why `describe` Matters

When working with large EPL codebases alongside an LLM, a `describe`-only summary of the file communicates full intent without requiring the LLM to read every line. Each `describe` is a **checkpoint** — a saved state of what the code does at that point.

---

## Reserved Keywords

The following words have special meaning in EPL and cannot be used as variable names.

### Core
`set` `update` `as` `to` `by` `from` `with` `and` `or` `not` `in` `of`

### Output / Input
`show` `display` `provide` `wait` `user` `input` `entry`

### Control Flow
`if` `then` `otherwise` `else` `while` `repeat` `until` `for` `each` `break` `continue` `return`

### Loop Variants
`times` `indefinitely` `down`

### Arithmetic
`plus` `minus` `times` `divided` `modulo` `power`

### Comparison
`is` `equals` `not` `greater` `less` `than` `at` `least` `most`

### Advanced Operators
`is any of` `is none of` `is not` `not equals` `greater than` `less than` `at least` `at most` `divided by` `power of`

### Activity / Documentation
`describe` `check` `that` `attempt`

### Collections
`add` `empty` `item` `items` `length` `size`

### Values
`true` `false` `null`

### System
`execute` `system` `command` `windows`

---

## Operator Precedence

EPL follows standard BODMAS/PEMDAS rules. Higher rows bind tighter.

| Priority | Operators                                        |
|----------|--------------------------------------------------|
| 1 (high) | `( )` — parentheses                             |
| 2        | `power of`                                       |
| 3        | `times` `divided by` `modulo`                   |
| 4        | `plus` `minus`                                   |
| 5        | `greater than` `less than` `at least` `at most` `equals` `is` `is not` `is any of` `in` |
| 6        | `not`                                            |
| 7        | `and`                                            |
| 8 (low)  | `or`                                             |

### Important Note on String Concatenation

The `+` symbol binds at the same level as `plus`. When mixing arithmetic and concatenation, store computed values first:

```epl
# This concatenates "Score: " + x, then adds y to the string
show "Score: " + x plus y       # may produce unexpected result

# Correct approach — compute first, display second
set result as x plus y
show "Score: " + result         # always correct
```

---

## Complete Example

The following is a complete working EPL program demonstrating all major features of v1.0.

```epl
### EPL Calculator v1.0 ###

describe calculator as "A simple CLI calculator with operation history"

set history as []
set running as true

describe add as "Returns sum of x and y"
to add, return x plus y!

describe subtract as "Returns x minus y"
to subtract, return x minus y!

describe multiply as "Returns product of x and y"
to multiply, return x times y!

describe divide as "Returns x divided by y, error on zero"
to divide
    if y equals 0 then
        return "Error: Division by zero"
    !
    return x divided by y
!

while running
    show "--- EPL Calculator ---"
    show "1. Add"
    show "2. Subtract"
    show "3. Multiply"
    show "4. Divide"
    show "5. View History"
    show "6. Exit"

    provide choice as user entry from "Select an option (1-6): "

    if choice is "6" then
        show "Goodbye!"
        update running as false
        continue
    !

    if choice is "5" then
        show "--- Calculation History ---"
        if history is empty then
            show "No records yet."
        !
        for each record in history
            show record
        !
        continue
    !

    if choice is any of "1", "2", "3", "4" then
        provide n1 as user entry from "Enter first number: "
        provide n2 as user entry from "Enter second number: "

        set result as 0
        set op as "?"

        if choice is "1" then
            update result as n1 plus n2
            update op as "+"
        !
        if choice is "2" then
            update result as n1 minus n2
            update op as "-"
        !
        if choice is "3" then
            update result as n1 times n2
            update op as "*"
        !
        if choice is "4" then
            if n2 equals 0 then
                show "Error: Cannot divide by zero"
                continue
            !
            update result as n1 divided by n2
            update op as "/"
        !

        set record as n1 + " " + op + " " + n2 + " = " + result
        show "Result: " + record
        add record to history
    otherwise
        show "Invalid selection. Choose 1 to 6."
    !
!
```

---

## What's Coming in v1.1

EPL v1.0 is the foundation. The following features are planned for v1.1:

**Smart Collections** — lists that know their own statistics:
```epl
set prices as [10, 20, 30, 45, 12]
show average of prices
show maximum of prices
show minimum of prices
show total of prices
```

**Smart Filter** — declarative data querying:
```epl
set adults as people where age is greater than 18
set cheap as prices where price is at most 20
```

**Richer Types** — dates, durations, formatted numbers:
```epl
set today as current date
set deadline as 7 days from today
```

**Connection Type** — relationship mapping:
```epl
link customer to store
link store to owner
```

---

*EPL v1.0 — Built from scratch in C. Tokenizer → Parser → AST → Interpreter → REPL.*
*Designed by a human. For humans.*
