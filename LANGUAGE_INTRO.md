# PatternLang

## Introduction

PatternLang is a simple, high-level programming language designed to help beginners understand the core concepts of compilers, syntax design, semantic analysis, intermediate code generation, and optimization. It was created as part of a compiler construction project, focusing on clarity, structured programming, and ease of parsing.

PatternLang supports variables, functions, conditions, loops, expressions, input/output, and recursive procedures—giving it enough expressive power for meaningful programs while keeping the language small and educational. As an added feature, it includes built-in pattern functions for ASCII art generation and math utility functions.

## Purpose

PatternLang allows you to:

- Learn compiler construction by studying a complete, working compiler pipeline
- Write general-purpose programs with variables, functions, and control flow
- Perform arithmetic and logical operations
- Implement recursive algorithms
- Use conditional logic and loops for complex behavior
- Generate ASCII art patterns programmatically (as a language feature)
- Perform mathematical computations with built-in math functions
- Accept user input and display results

## Design Philosophy

- **Educational**: Designed to teach compiler construction and language design principles
- **Accessible**: Simple syntax that beginners can understand and extend
- **Type-Safe**: Strong typing for variables (int, bool, string)
- **Readable**: Clean, pseudo-code-like syntax inspired by C and Pascal
- **Executable**: Programs compile to intermediate code (TAC), get optimized, and translate to C++
- **General-Purpose**: Core language handles typical programming tasks; pattern/math functions are bonus features

## Language Scope

### What PatternLang Can Do

- Declare typed variables (int, bool, string)
- Perform arithmetic operations (+, -, *, /, %)
- Perform logical operations (&&, ||, !)
- Define reusable functions with parameters and return values
- Implement recursive functions
- Use conditional logic (if/else)
- Repeat operations with loops (for, while)
- Call built-in pattern functions (pyramid, diamond, box, stairs, etc.)—optional feature
- Call built-in math functions (max, min, abs, pow, sqrt, rangeSum, isPrime, factor, etc.)—optional feature
- Display values with print statements
- Accept user input with input statements
- Write meaningful programs with multiple functions and complex control flow

### What PatternLang Cannot Do

- File I/O operations
- Network operations
- Complex data structures (arrays are not fully implemented)
- Exception handling
- Advanced string manipulation beyond basic literals
- Reflection or metaprogramming

## Type System

PatternLang has 3 built-in types:

| Type | Purpose | Example Values |
|------|---------|-----------------|
| `int` | Integer values | `count = 5` |
| `bool` | Boolean values | `flag = true` |
| `string` | Text strings | `symbol = "*"` |

## Sample Syntax

### Variable Declaration

```
int count = 10;
bool flag = true;
string symbol = "*";
```

### Arithmetic Operations

```
int result = 5 + 3;
int product = 2 * 4;
int quotient = 10 / 2;
int remainder = 10 % 3;
```

### Conditional Statements

```
if (x > 5) {
    print x;
} else {
    print 0;
}
```

### Loops

```
for i = 1 to 10 {
    print i;
}

while (count > 0) {
    print count;
}
```

### Function Declaration

```
func add(int a, int b) {
    return a + b;
}

func fibonacci(int n) {
    if (n <= 1) {
        return n;
    } else {
        return fibonacci(n - 1) + fibonacci(n - 2);
    }
}
```

### Function Calls

```
int sum = add(5, 3);
pyramid(5);
diamond(3);
```

### Built-in Pattern Functions

```
# Generate a pyramid pattern of size n
pyramid(5);

# Generate a diamond pattern of size n
diamond(4);

# Generate a box pattern with character, width, and height
box("*", 6, 4);

# Generate a staircase pattern with size and character
stairs(5, "#");
```

### Built-in Math Functions

```
int max_val = max(10, 20);           # Returns 20
int min_val = min(7, 3);             # Returns 3
int abs_val = abs(-42);              # Returns 42
int power = pow(2, 5);               # Returns 32
int sqrt_val = sqrt(49);              # Returns 7
int sum = rangeSum(10);              # Returns 1+2+...+10 = 55
bool is_prime = isPrime(13);         # Returns true
```

### I/O Operations

```
# Display a value
print x;

# Print a newline
newline;

# Accept user input
input count;
```

## Operations

### Print Statement
```
print 42;           # Output: 42
print x;            # Output: value of x
```

### Input Statement
```
input n;            # Prompts user to enter value for n
```

### Newline
```
newline;            # Output: newline character
```

## Comments

```
# This is a single-line comment
# Everything after # until end of line is ignored
```

## Example Program: General-Purpose Program

```
# Calculate factorial recursively
func factorial(int n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

# Main execution
int x = 5;
int result = factorial(x);
print result;
newline;
```

## Example Program: Using Pattern Functions

```
# Generate a pyramid pattern with customizable size
int n = 5;
print n;
newline;
pyramid(n);
newline;
diamond(3);
newline;
```

## Example Program: Using Math Functions

```
# Demonstrate built-in math functions
print max(10, 20);
newline;
print min(7, 3);
newline;
print pow(2, 5);
newline;
print sqrt(49);
newline;
```

## Error Messages

### Undeclared Variable
```
Semantic Error: Variable 'count' not declared
```

### Type Mismatch
```
Semantic Error: Cannot assign string to int variable 'x'
```

### Division by Zero
```
Runtime Error: Division by zero
```

### Function Not Found
```
Semantic Error: Function 'unknown_func' is not defined
```

### Missing Return Value
```
Semantic Error: Function 'getValue' must return a value
```

### Parameter Mismatch
```
Semantic Error: Function 'add' expects 2 parameters, got 1
```

## Compilation & Execution Pipeline

1. **Lexical Analysis**: Source code tokenized into tokens (keywords, identifiers, literals, operators, punctuation)
2. **Parsing**: Tokens parsed into Abstract Syntax Tree (AST) using recursive descent parser
3. **Semantic Analysis**: Type checking, symbol table construction, scope validation, function resolution
4. **Intermediate Representation**: Three-Address Code (TAC) generation
5. **Optimization**: Dead code elimination, constant folding, strength reduction
6. **Code Generation**: Translation to C++ source code
7. **Compilation**: C++ code compiled to executable using g++
8. **Execution**: Program runs and produces output

## Getting Started

### Example 1: Simple Pattern
```
pyramid(5);
newline;
diamond(3);
```

### Example 2: Using Variables and Loops
```
int n = 10;
for i = 1 to n {
    print i;
}
```

### Example 3: Functions and Math
```
func factorial(int n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

print factorial(5);
```

### Example 4: Pattern with Input
```
input size;
pyramid(size);
newline;
diamond(size / 2);
```

## Language Features at a Glance

| Feature | Supported | Example |
|---------|-----------|---------|
| Variables (int, bool, string) | ✓ | `int x = 5;` |
| Functions with return | ✓ | `func add(int a, int b) { ... }` |
| Recursion | ✓ | `func fib(int n) { ... }` |
| Conditionals (if/else) | ✓ | `if (x > 0) { ... }` |
| Loops (for, while) | ✓ | `for i = 1 to 10 { ... }` |
| Arithmetic operators | ✓ | `+, -, *, /, %` |
| Logical operators | ✓ | `&&, \|\|, !` |
| Comparison operators | ✓ | `==, !=, <, >, <=, >=` |
| User Input | ✓ | `input x;` |
| Output (print) | ✓ | `print x;` |
| Comments | ✓ | `# comment` |
| **Pattern Functions** (optional) | ✓ | `pyramid(5);` |
| **Math Functions** (optional) | ✓ | `pow(2, 5);` |
| Arrays | ✗ | Not implemented |
| File I/O | ✗ | Not supported |
| Exception Handling | ✗ | Not supported |

---

**PatternLang**: An educational compiler and simple, structured programming language—by your project team.
