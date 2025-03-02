Objective:
The goal of this assignment is to thoroughly analyze and understand the structure, functionality, and
implementation details of the C4 compiler. By the end of this assignment, you will be able to explain
how the code works, identify its key components, and reason about its design decisions.
Tasks:
1. Annotate the Code:
• Download the C4 code and add detailed comments to every function, block, and nontrivial line of code.
• Your comments should explain:
• What the code does.
• Why it is written in a particular way.
• How it fits into the overall structure of the compiler.
• Save the annotated code as c4_annotated.c.
2. Draw a Detailed Diagram:
• Create a diagram that represents the architecture and data flow of the C4 compiler.
Include:
• The main components (e.g., lexer, parser, virtual machine, etc.).
• How data moves between these components.
• Key data structures (e.g., tokens, instructions, stack, etc.).
• Save the diagram as c4_diagram.png or c4_diagram.pdf.
3. Collect Code Statistics:
• Use static and dynamic analysis tools to collect the following statistics about the C4 code:
• Lines of code (LOC) for the entire program and for individual functions.
• Cyclomatic complexity of each function.
• Number of functions and their sizes (in LOC).
• Number of global variables and their usage.
• Number of unique tokens and their frequency.
• Number of branches, loops, and their nesting levels.
• Memory usage patterns (e.g., stack vs. heap allocation).
• Write a summary of your findings in a file named c4_statistics.txt.
4. Explain Key Algorithms:
• Write a short report (1-2 pages) explaining the following algorithms or concepts used in C4:
• The lexical analysis process: How does the code identify and tokenize input?
• The parsing process: How does the code construct an abstract syntax tree (AST) or
equivalent representation?
• The virtual machine implementation: How does the code execute the compiled
instructions?
• The memory management approach: How does the code handle memory allocation and
deallocation?
• Save the report as c4_report.pdf.
5. Answer Conceptual Questions:
• Provide written answers to the following questions:
1. What is the purpose of the next() function, and how does it contribute to the compilation
process?
2. How does C4 handle symbol resolution (e.g., variables, functions)?
3. What are the limitations of C4 as a compiler? What features of C does it not support?
4. How does C4 achieve self-hosting (i.e., compiling itself)? What are the implications of this
design?
• Save the answers in a file named c4_answers.txt.
6. Experiment with the Code (Optional; Bonus 10%):
• Compile and run the C4 compiler on your local machine.
• Modify the code to add a new feature or change an existing behavior. For example:
• Add support for a new operator (e.g., % for modulus).
• Change the way the virtual machine handles a specific instruction.
• Write a short explanation of your changes and how they affect the compiler’s behavior.
• Save the modified code as c4_modified.c and the explanation
as c4_modification_explanation.txt.
