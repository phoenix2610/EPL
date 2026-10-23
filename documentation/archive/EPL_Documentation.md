# EPL (English Programming Language) v1.0 Documentation

Welcome to the official documentation for the **English Programming Language (EPL)**. EPL is designed to read like natural English, removing the syntactic friction found in traditional languages while retaining full programming power.

---

## 1. Data Types & Literals
EPL supports the following primitive data types:

- **Number**: Integers or floating-point numbers (`42`, `19.99`).
- **String**: Text enclosed in double quotes (`"Hello World"`).
- **Boolean**: True or false values (`true`, `false`).
- **Null**: Represents the absence of a value (`null`).
- **List/Array**: An ordered collection of items enclosed in brackets (`[1, 2, "apple"]`).

---

## 2. Variables

### Declaration
Use the `set` keyword to declare a new variable or initialize it.
```epl
set name as "Alice"
set age as 30
```

### Assignment/Updating
Use the `update` keyword (or `set`) to change an existing variable. You can also calculate new values based on the current value.
```epl
update age as age plus 1
set salary by 500  # Syntactic sugar for modifying a variable
```

---

## 3. Input & Output

### Output
Use `show` or `display` to print values to the console.
```epl
show "Hello, " + name
display age
```

### Input
Use `wait for user input` or `provide` to capture console input.
```epl
set user_name as wait for user input
```

---

## 4. Arithmetic Operations
Mathematical expressions are written with natural English words.

* **Addition**: `plus` (e.g., `x plus y`)
* **Subtraction**: `minus` (e.g., `x minus y`)
* **Multiplication**: `times` (e.g., `x times y`)
* **Division**: `divided by` (e.g., `x divided by y`)
* **Modulo**: `modulo` (e.g., `x modulo y`)
* **Exponentiation**: `power of` (e.g., `x power of 2`)

*(Note: Traditional operators like `+` are also supported, especially for string concatenation).*

---

## 5. Logical & Comparison Operators

### Comparisons
* **Equality**: `equals` or `is` (e.g., `x is 10`)
* **Inequality**: `not equals` or `is not`
* **Greater/Less**: `greater than`, `less than`
* **Inclusive Range**: `at least` (>=), `at most` (<=)

### Logical Connectives
* **And**: `and` (e.g., `x is 5 and y is 10`)
* **Or**: `or` (e.g., `x is 5 or x is 6`)
* **Not**: `not` (e.g., `not true`)

### Advanced Comparisons
* **List Inclusion**: `in`, `is any of`, `is none of`
  ```epl
  if x is any of 1, 2, 3 then ... !
  ```

---

## 6. Control Flow

Blocks in EPL are terminated with an exclamation mark (`!`). 

### If / Otherwise
```epl
if age is greater than 17 then
    show "Adult"
otherwise
    show "Minor"
!
```

### While Loop
Executes a block as long as the condition remains true.
```epl
while count is less than 10
    update count as count plus 1
!
```

### For Loop (Numeric)
Iterates through a numeric range.
```epl
for i from 1 to 5
    show i
!
```

### For Each Loop (Lists)
Iterates over elements in a collection.
```epl
for each item in [1, 2, 3]
    show item
!
```

### Repeat Loop
Repeats a block a specific number of times.
```epl
repeat 5 times
    show "Hello"
!
```
*(Also supports `repeat indefinitely` until a `break` is encountered).*

### Loop Control
* **`break`**: Exits the current loop entirely.
* **`continue`**: Skips the rest of the current iteration and moves to the next.

---

## 7. Collections & Lists
Lists are comma-separated values inside brackets.

### Length / Size
```epl
set my_list as [1, 2, 3]
show length of my_list
```

### Indexing
Accessing an item inside a collection:
```epl
show item 0 of my_list
```

### Modification
```epl
add "cherry" to my_list
set my_list as empty
```

---

## 8. Activities (Functions)
Functions in EPL are called **Activities**. They are defined using the `to` keyword and ended with `!`.

### Definition
```epl
to greet
    show "Hello World!"
!
```

### Returning Values
```epl
to get_magic_number
    return 42
!
```

### One-line Activities
```epl
to say_hello, return "Hello!"
```

---

## 9. Error Handling
EPL allows for graceful failure via the `attempt` block.

```epl
attempt
    set result as 10 divided by 0
if fails
    show "Cannot divide by zero!"
!
```

---

## 10. System Commands
You can interact with the underlying OS using system commands.

```epl
execute windows command "dir"
```

---

## Summary of Keywords
- **Variables**: `set`, `update`, `as`, `by`
- **Logic**: `if`, `then`, `otherwise`, `else`
- **Loops**: `while`, `repeat`, `until`, `for`, `each`, `from`, `to`, `down`, `times`, `indefinitely`
- **Math/Logic**: `plus`, `minus`, `times`, `divided by`, `modulo`, `power of`, `is`, `equals`, `not`, `and`, `or`, `greater than`, `less than`, `at least`, `at most`
- **Collections**: `in`, `is any of`, `length of`, `size of`, `add`, `empty`, `item`, `items`
- **Activities/Functions**: `to`, `return`, `describe`
- **System**: `execute`, `system`, `command`, `windows`
- **Block End**: `!`
