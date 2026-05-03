<div align="center">
  <img src="documentation/EPL_logo.png" alt="EPL Logo" width="200"/>
  <h1>EPL — English Programming Language</h1>
  <p><em>"Your thoughts are the syntax."</em></p>

  [![Version](https://img.shields.io/badge/version-1.5-blue.svg)](#)
  [![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)](#)
  [![Language](https://img.shields.io/badge/language-C11-orange.svg)](#)
  
  <p>
    An interpreted language where English is not just readable — it is the source code. <br />
    Built for humans first, executed by a C interpreter second.
  </p>
</div>

---

## 📖 Table of Contents
- [Philosophy & The "Vibe Coder" Problem](#-philosophy--the-vibe-coder-problem)
- [Quick Start](#-quick-start)
- [Interactive Syntax Guide](#-interactive-syntax-guide)
- [The Full Stack Vision](#-the-full-stack-vision)

---

## 🧠 Philosophy & The "Vibe Coder" Problem

EPL exists to solve the modern **vibe coder problem**.
When developers use AI to generate all their code, they lose their mental model of the system. In EPL, your thought process *is* the source code.

```text
Human thought → EPL → C Interpreter → Execution
                ↑
         You always stay here.
```

### Why EPL?
EPL boasts superior **semantic token density**. Every token is signal, making EPL **5-6x more efficient** for LLMs to read than standard Python. 
With our v1.5 feature **`.EPD` (Semantic Indexes)**, LLMs can instantly understand your codebase without reading the raw logic.

<details>
<summary><strong>🔍 Click to see how Semantic Indexing (.EPD) works</strong></summary>

Instead of parsing thousands of lines of logic, the EPL compiler generates `.EPD` index files. 
An LLM navigating a 10,000-line codebase reads the 800-token `.EPD` file, finds the exact function, and only reads what it needs to modify.

**Generation:**
```powershell
.\epl.exe --describe program.EP      # Generates program.EPD
```
</details>

---

## 🚀 Quick Start

### Build from Source
EPL is incredibly fast and has zero external dependencies outside of standard C.
```bash
gcc -Wall -std=c11 -o epl main.c lexer.c token.c ast.c parser.c interpreter.c repl.c codegen.c binding.c describe.c -lm
```

### Run a File
```powershell
.\epl.exe program.EP           # Logic execution
.\epl.exe dashboard.EPF        # Generates Live HTML Dashboards
```

### Launch the REPL
```powershell
.\epl.exe
```

---

## ⚡ Interactive Syntax Guide

*Click on any section below to expand and explore the EPL syntax!*

<details>
<summary><strong>📦 Variables & Smart Collections</strong></summary>

EPL uses `set` for initialization and `update` for mutation.
```epl
set name as "Phoenix"
set scores as [85, 92, 78, 95]

update name as "Tathya"
```

EPL supports advanced, readable filtering natively:
```epl
set passed as scores where score at least 60
show average of passed
show "Highest: " + maximum of scores
```
</details>

<details>
<summary><strong>🔀 Conditionals & Loops</strong></summary>

Clean and readable control flow:
```epl
if score at least 60 then
    show "Passed"
otherwise if score at least 50 then
    show "Make-up Exam"
otherwise
    show "Fail"
!
```

```epl
for each student in students
    show student
!

repeat 5 times with i
    show "Step " + i
!
```
</details>

<details>
<summary><strong>⚙️ Activities (Functions) & Error Handling</strong></summary>

Functions are defined as "activities". They map to human tasks.
```epl
describe divide as "Divides x by y safely"
to divide
    check that y is not 0
    return x divided by y
!

attempt
    set result as 10 divided by 0
if fails then
    show "Math error prevented."
!
```
</details>

<details>
<summary><strong>🕸️ Graph Data Structures (v1.3)</strong></summary>

Graph structures are built right into the language.
```epl
set network as graph

link customer to store in network
link store to warehouse in network

set path as route from customer to warehouse in network
show path
```
</details>

---

## 🌐 The Full Stack Vision

EPL handles the entire stack in one human-readable syntax:

| Extension | Purpose | Output |
|-----------|---------|---------|
| **`.EP`** | Logic, backend computation | Terminal Execution |
| **`.EPF`** | Frontend Dashboards (v1.4) | Live `.html` files |
| **`.EPD`** | Semantic Index (v1.5) | Read-only LLM context |

<details>
<summary><strong>🎨 See an .EPF Frontend Example</strong></summary>

EPL generates complete HTML dashboards with Chart.js using purely declarative English:
```epf
make page saved as "dashboard.html" styled as "dark"

    heading "My Dashboard"
    paragraph "Welcome to the EPL-powered dashboard."
    
    section "Charts"
        make line chart for x axis as months and y axis as revenue titled "Revenue"
    !
!
```
</details>

---
<div align="center">
</div>
