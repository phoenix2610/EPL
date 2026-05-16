# EPF Frontend Engine Guide

The EPL compiler (v1.7) includes a highly advanced, premium frontend generation engine that converts natural English instructions into production-ready, interactive web prototypes. 

This document explains how the `.epf` (English Prototype Format) compilation pipeline works, the keywords involved, and how to structure your files.

---

## The Compilation Pipeline

When you run `epl.exe filename.epf`, the compiler executes the following phases:
1. **Lexing & Parsing**: The pure C recursive descent parser reads the English statements and generates an AST (Abstract Syntax Tree).
2. **Interpretation**: The AST is executed, building a virtual memory state containing variables, lists, and pages.
3. **Code Generation (`codegen.c`)**: If `make page` or other frontend elements are encountered, the compiler generates a responsive, multipage HTML/CSS/JS frontend using premium design tokens.

### The Shell Injection Architecture

The compiler automatically links premium assets into every generated page:
- **`css/epl_base.css`**: Provides high-fidelity modern typography (Syne, Space Mono), a custom cursor, glowing animations, and styling for standard EPL components (`stat`, `chart`, `report`).
- **`js/app.js`**: Injects scroll animations, nav interaction, and cursor movement logic.

You do **not** need to manually define navigation or basic styles. The compiler automatically builds a top-level `<nav>` with links to all pages in your project.

---

## Core EPF Keywords

### 1. `make page`
Creates a new HTML file in the project. If multiple pages are created, the compiler automatically generates a unified navigation bar.
**Syntax:**
```text
make page saved as "filename.html" styled as "dark"
```

### 2. `make stat`
Generates a responsive statistical block with hover effects and premium typography.
**Syntax:**
```text
make stat labelled "LINES OF CODE" valued at 25000
```

### 3. `make chart`
Generates an interactive Chart.js canvas. Note: The compiler seamlessly handles `canvas3d` for Three.js integrations as well.
**Syntax:**
```text
make chart named "Performance" plotting my_data
```

### 4. `make report`
Generates a structured table or report view. It utilizes `Space Mono` for tabular data alignment.
**Syntax:**
```text
make report using my_list
```

### 5. `make canvas3d`
Creates an interactive WebGL scene powered by Three.js (used in `orbital.epf`).
**Syntax:**
```text
make canvas3d named "orbital"
```

### 6. `paragraph`
Injects raw text or HTML directly into the page output. Useful for custom hero sections or inline assets.
**Syntax:**
```text
paragraph "<section class='hero'>...</section>"
```

---

## Styling & Responsiveness

The updated v1.7 compiler abandons the old rigid sidebar (`epl-side`) in favor of a modern, responsive top navigation bar (`.nav`).

Pages now utilize **Full Page Modulus** (`width: 100%; min-height: 100vh`), allowing hero sections to span from edge to edge without wasted margin space.

## Custom Assets
To modify the underlying theme, you can edit:
- `build/css/epl_base.css`
- `build/js/app.js`

Any modifications to these files will automatically apply to all `.epf` files compiled by the engine.
