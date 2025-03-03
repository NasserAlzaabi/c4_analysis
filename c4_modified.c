/*
	This is a minimal C compiler that can compile its own source code.
	It includes a tokenizer, parser, and a virtual machine (VM) for execution.
	
	The three core functions:
	1. next()  - Tokenizes the input source code.
	2. expr()  - Parses and generates code for expressions.
	3. stmt()  - Parses and generates code for statements.

	This design follows a **single-pass** compilation approach, where source code
	is directly translated into executable instructions, rather than using an
	intermediate representation.
*/

#include <stdio.h>   // Standard I/O functions
#include <stdlib.h>  // Memory allocation and program exit functions
#include <memory.h>  // Memory handling utilities
#include <unistd.h>  // System calls (file handling, process control)
#include <fcntl.h>   // File control options

// Force all integer values to be 64-bit (long long).
// This ensures compatibility across different platforms.
#define int long long 

// Global pointers used for managing source code parsing and execution.
char *p,  *lp,  // p: Pointer to the current position in source code
               // lp: Pointer to the start of the current line in source code
     *data;     // Points to memory where global/static data is stored

// Pointers for emitted machine code and symbol table.
int *e,  *le,   // e: Points to the current position in generated code
               // le: Last emitted instruction (used for debugging)
    *id,       // Points to the currently parsed identifier in the symbol table
    *sym;      // Symbol table (stores identifiers, their types, and values)

// Token-related variables
int tk,        // Holds the current token being processed
    ival,      // Holds integer values for numeric tokens
    ty,        // Tracks the data type of an expression
    loc,       // Tracks local variable stack offset
    line,      // Stores the current line number (for error reporting)
    src,       // Debugging flag to print source code while compiling
    debug;     // Debugging flag to print executed instructions

// Token types and symbol classifications
// The order of operators in the enum ensures correct precedence in parsing.
enum {
  Num = 128, // Numeric literals (integer constants)
  Fun, Sys, Glo, Loc, Id, // Function, system call, global var, local var, identifier
  Char, Else, Enum, If, Int, Return, Sizeof, While, // Keywords
  Assign, Cond, Lor, Lan, Or, Xor, And, // Operators in precedence order
  Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, // Arithmetic and bitwise ops
  Inc, Dec, Brak // Increment, decrement, and bracket operators
};

// Virtual Machine (VM) opcodes
// These represent the core instructions used in generated code.
enum { LEA, IMM, JMP, JSR, BZ, BNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PSH, 
       OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
       OPEN, READ, CLOS, PRTF, MALC, FREE, MSET, MCMP, EXIT };

// Data types (used for variable declarations and type checking)
enum { CHAR, INT, PTR };

// Symbol table structure
// Since we can't use structs, identifiers are stored as arrays with fixed offsets.
enum { Tk, Hash, Name, Class, Type, Val, HClass, HType, HVal, Idsz };

// ------------------------------------------------------------------------------------
// The next() function (Tokenizer) is responsible for breaking the source code into tokens.
// It scans characters from the source, identifies keywords, operators, numbers, and symbols.
// ------------------------------------------------------------------------------------

void next()
{
  char *pp; //Temporary pointer that stores identifiers starting position

  while (tk = *p) { //Read token in the code
    ++p; //Move to the next character
    if (tk == '\n') { //If current token is new line
      if (src) { //If debugging then print the source code
        printf("%d: %.*s", line, p - lp, lp); //Print the current line of code
        lp = p; //Store start of next lien of code
        while (le < e) { //Loop over emitted code
          printf("%8.4s", &"LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,"
                           "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
                           "OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,"[*++le * 5]); //print instruction names
          if (*le <= ADJ) printf(" %d\n", *++le); else printf("\n"); //print instruction operands
        }
      }
      ++line; //increment line number
    }
    else if (tk == '#') { //If current character is a #
      while (*p != 0 && *p != '\n') ++p; //skip until end of line or code
    }
    else if ((tk >= 'a' && tk <= 'z') || (tk >= 'A' && tk <= 'Z') || tk == '_') { //If its alphabetic or _
      pp = p - 1; //Store the start of identifier
      while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '_') //If its alphanumeric or _
        tk = tk * 147 + *p++; //Generates a hash for the identifier
      tk = (tk << 6) + (p - pp);
      id = sym; //Start symbol table tree
      while (id[Tk]) { //Loop over symbol table
        if (tk == id[Hash] && !memcmp((char *)id[Name], pp, p - pp)) { tk = id[Tk]; return; } //If identifier exists, return the token
        id = id + Idsz;
      }
      id[Name] = (int)pp; //Store new identifier in symbol table
      id[Hash] = tk; //Save the hash of identifier
      tk = id[Tk] = Id; //Mark as an identifier
      return; //Exit function
    }
    else if (tk >= '0' && tk <= '9') { //If the token is a number
      if (ival = tk - '0') { while (*p >= '0' && *p <= '9') ival = ival * 10 + *p++ - '0'; } //Convert token and characters into integer values
      else if (*p == 'x' || *p == 'X') {  //if the current character is an x or X
        while ((tk = *++p) && ((tk >= '0' && tk <= '9') || (tk >= 'a' && tk <= 'f') || (tk >= 'A' && tk <= 'F'))) //loop over the hex values
          ival = ival * 16 + (tk & 15) + (tk >= 'A' ? 9 : 0); //Convert hex into integers
      }
      else { while (*p >= '0' && *p <= '7') ival = ival * 8 + *p++ - '0'; } //Convert octal into integers
      tk = Num; //mark token as number
      return; //exit function
    }
    else if (tk == '/') { //If token is a '/'
      if (*p == '/') { //Check if its a comment
        ++p; 
        while (*p != 0 && *p != '\n') ++p; //Skip the line
      }
      else { 
        tk = Div; //Token is division
        return; //Exit function
      }
    }
    else if (tk == '\'' || tk == '"') { //If token is ' or "
      pp = data; //Store starting address for string
      while (*p != 0 && *p != tk) { //Loop until end of quote
        if ((ival = *p++) == '\\') { //check for a '\'
          if ((ival = *p++) == 'n') ival = '\n'; //If next character is n, its new line
        }
        if (tk == '"') *data++ = ival; //If its a string store string in data
      }
      ++p;
      if (tk == '"') ival = (int)pp; else tk = Num; //If its a string store address, if character store as number
      return;
    }
	//Handle operators
    else if (tk == '=') { if (*p == '=') { ++p; tk = Eq; } else tk = Assign; return; }
    else if (tk == '+') { if (*p == '+') { ++p; tk = Inc; } else tk = Add; return; }
    else if (tk == '-') { if (*p == '-') { ++p; tk = Dec; } else tk = Sub; return; }
    else if (tk == '!') { if (*p == '=') { ++p; tk = Ne; } return; }
    else if (tk == '<') { if (*p == '=') { ++p; tk = Le; } else if (*p == '<') { ++p; tk = Shl; } else tk = Lt; return; }
    else if (tk == '>') { if (*p == '=') { ++p; tk = Ge; } else if (*p == '>') { ++p; tk = Shr; } else tk = Gt; return; }
    else if (tk == '|') { if (*p == '|') { ++p; tk = Lor; } else tk = Or; return; }
    else if (tk == '&') { if (*p == '&') { ++p; tk = Lan; } else tk = And; return; }
    else if (tk == '^') { tk = Xor; return; }
    else if (tk == '%') { tk = Mod; return; }
    else if (tk == '*') { tk = Mul; return; }
    else if (tk == '[') { tk = Brak; return; }
    else if (tk == '?') { tk = Cond; return; }
    else if (tk == '~' || tk == ';' || tk == '{' || tk == '}' || tk == '(' || tk == ')' || tk == ']' || tk == ',' || tk == ':') return;
  }
}

/*
	expr(int lev) -- Expression Parsing function
	Parses and generates code for expressions
	based on precedence
*/
void expr(int lev)
{
  int t, *d; //Temp vars, t for types, d* for jumps and branches

  if (!tk) { printf("%d: unexpected eof in expression\n", line); exit(-1); } //Error output when token is empty
  else if (tk == Num) { *++e = IMM; *++e = ival; next(); ty = INT; } //If token is a number, IMM instruction with a number
  else if (tk == '"') { //If token is string literal
    *++e = IMM; *++e = ival; next(); //IMM instruction with string address
    while (tk == '"') next(); //Skip repeated "
    data = (char *)((int)data + sizeof(int) & -sizeof(int)); ty = PTR; //Align memory for storing string data
  }
  // ------------------------------------------------------------------------------------
  // HERE IS MY MODIFICATION

  else if (tk == '~') { 
    next(); 
    expr(Inc);  // Parse the operand
    *++e = PSH; 
    *++e = IMM; 
    *++e = -1;   // Load -1 onto the stack
    *++e = XOR;  // Perform bitwise NOT (XOR with -1)
    ty = INT;    // Result is an integer
  }
  // ------------------------------------------------------------------------------------

  else if (tk == Sizeof) { //If token is Sizeof
    next();
	if (tk == '(') next(); //If '(' follows Sizeof, get next token
	else { //Else print error and exit
		printf("%d: open paren expected in sizeof\n", line);
		exit(-1);
	} 
    ty = INT;
	if (tk == Int) next(); //If token is of type Int, get next token
	else if (tk == Char) { next(); ty = CHAR; } //If token is of type Char, get next token and set ty to CHAR
    while (tk == Mul) { next(); ty = ty + PTR; } //Handle pointer types
    if (tk == ')') next(); //If Sizeof ends with ')', next token
	else { printf("%d: close paren expected in sizeof\n", line); exit(-1); } //Else print error and exit
    *++e = IMM; *++e = (ty == CHAR) ? sizeof(char) : sizeof(int); //IMM instruction with size of type
    ty = INT; //Reset type to int
  }
  else if (tk == Id) { //If token is an identifier
    d = id; next(); //Store identifier info and go to next token
    if (tk == '(') { //If its a function call
      next();
      t = 0; //Reset argument counter
      while (tk != ')'){ expr(Assign); *++e = PSH; ++t; if (tk == ',') next(); } //Process function arguments
      next(); //Go to next token
      if (d[Class] == Sys) *++e = d[Val]; //If system function (like printf), emit the instruction
      else if (d[Class] == Fun) { *++e = JSR; *++e = d[Val]; } //If user function, emit JSR (jump)
      else { printf("%d: bad function call\n", line); exit(-1); } //Else print error
      if (t) { *++e = ADJ; *++e = t; } //Adjust stack for the functions arguments
      ty = d[Type]; //Set return type
    }
    else if (d[Class] == Num) { *++e = IMM; *++e = d[Val]; ty = INT; } //If identifier is a constant, emit IMM instruction
    else { //Else its a variable
      if (d[Class] == Loc) { *++e = LEA; *++e = loc - d[Val]; } //If local variable, emit LEA
      else if (d[Class] == Glo) { *++e = IMM; *++e = d[Val]; } //If global variable, emit IMM
      else { printf("%d: undefined variable\n", line); exit(-1); } //Else error 
      *++e = ((ty = d[Type]) == CHAR) ? LC : LI; //Load variable value, char or int
    }
  }
  else if (tk == '(') { //If token is (
    next(); //Go to next token
    if (tk == Int || tk == Char) { //If token is either int or char
      t = (tk == Int) ? INT : CHAR; next(); //Store type then go to next token
      while (tk == Mul) { next(); t = t + PTR; } //Handle the pointer typecase
      if (tk == ')') next(); else { printf("%d: bad cast\n", line); exit(-1); } //Ensure it ends with ) else error
      expr(Inc); //Parse the expression being typecast
      ty = t; //Set expression type
    }
    else { //If not typecast, normal expression
      expr(Assign); //Parse expression
      if (tk == ')') next(); else { printf("%d: close paren expected\n", line); exit(-1); } //Ensure it ends with )
    }
  }
  else if (tk == Mul) { //If token is *
    next(); expr(Inc); //parse the expression after *
    if (ty > INT) ty = ty - PTR; else { printf("%d: bad dereference\n", line); exit(-1); } //Ensures its a pointer, else error
    *++e = (ty == CHAR) ? LC : LI; //Emit LC or LI if char or int
  }
  else if (tk == And) { //If token is &
    next(); expr(Inc); //Parse expression after &
    if (*e == LC || *e == LI) --e; else { printf("%d: bad address-of\n", line); exit(-1); } //Remove LC or LI to keep address, error if & used on non-variable
    ty = ty + PTR; //Increment pointer
  }
  else if (tk == '!') { next(); expr(Inc); *++e = PSH; *++e = IMM; *++e = 0; *++e = EQ; ty = INT; } //If token !, compare value to zero
  else if (tk == '~') { next(); expr(Inc); *++e = PSH; *++e = IMM; *++e = -1; *++e = XOR; ty = INT; } //If token ~, XOR with -1 
  else if (tk == Add) { next(); expr(Inc); ty = INT; } //single addition, ignored, goes to next
  else if (tk == Sub) { //If token is -
    next(); *++e = IMM; //Emit IMM to load value
    if (tk == Num) { *++e = -ival; next(); } else { *++e = -1; *++e = PSH; expr(Inc); *++e = MUL; } //if its number negate it
    ty = INT; //Else, multiply by -1
  }
  else if (tk == Inc || tk == Dec) { //If token is Increment or Decrement
    t = tk; next(); expr(Inc); //Parse the expression after token
    if (*e == LC) { *e = PSH; *++e = LC; } //If its Char, load its value
    else if (*e == LI) { *e = PSH; *++e = LI; } //If its Int, load its value
    else { printf("%d: bad lvalue in pre-increment\n", line); exit(-1); } //Else error
    *++e = PSH; //Push value onto stack
    *++e = IMM; *++e = (ty > PTR) ? sizeof(int) : sizeof(char); //Load increment decrement size
    *++e = (t == Inc) ? ADD : SUB; //Performe addition or subtration
    *++e = (ty == CHAR) ? SC : SI; //Store the new value
  }
  else { printf("%d: bad expression\n", line); exit(-1); } //Else error

  while (tk >= lev) { // "precedence climbing" or "Top Down Operator Precedence" method
    t = ty; //Store current type in t
    if (tk == Assign) { //if token is =
      next(); //go to next token
      if (*e == LC || *e == LI) *e = PSH; else { printf("%d: bad lvalue in assignment\n", line); exit(-1); } //if token is Char or Int, load its value, else error
      expr(Assign); *++e = ((ty = t) == CHAR) ? SC : SI; //Parse right side expression
    }
    else if (tk == Cond) { //If token is ?
      next(); // go to next token
      *++e = BZ; d = ++e; //Emit branch for false case
      expr(Assign); //Parse first expression
      if (tk == ':') next(); else { printf("%d: conditional missing colon\n", line); exit(-1); } //If : follows the true expression, else error
      *d = (int)(e + 3); *++e = JMP; d = ++e; //Emit jump to skip false branch
      expr(Cond); //Parse second expression
      *d = (int)(e + 1); //Set jump to false branch
    }

	//Handle operations
    else if (tk == Lor) { next(); *++e = BNZ; d = ++e; expr(Lan); *d = (int)(e + 1); ty = INT; }
    else if (tk == Lan) { next(); *++e = BZ;  d = ++e; expr(Or);  *d = (int)(e + 1); ty = INT; }
    else if (tk == Or)  { next(); *++e = PSH; expr(Xor); *++e = OR;  ty = INT; }
    else if (tk == Xor) { next(); *++e = PSH; expr(And); *++e = XOR; ty = INT; }
    else if (tk == And) { next(); *++e = PSH; expr(Eq);  *++e = AND; ty = INT; }
    else if (tk == Eq)  { next(); *++e = PSH; expr(Lt);  *++e = EQ;  ty = INT; }
    else if (tk == Ne)  { next(); *++e = PSH; expr(Lt);  *++e = NE;  ty = INT; }
    else if (tk == Lt)  { next(); *++e = PSH; expr(Shl); *++e = LT;  ty = INT; }
    else if (tk == Gt)  { next(); *++e = PSH; expr(Shl); *++e = GT;  ty = INT; }
    else if (tk == Le)  { next(); *++e = PSH; expr(Shl); *++e = LE;  ty = INT; }
    else if (tk == Ge)  { next(); *++e = PSH; expr(Shl); *++e = GE;  ty = INT; }
    else if (tk == Shl) { next(); *++e = PSH; expr(Add); *++e = SHL; ty = INT; }
    else if (tk == Shr) { next(); *++e = PSH; expr(Add); *++e = SHR; ty = INT; }
    else if (tk == Add) {
      next(); *++e = PSH; expr(Mul);
      if ((ty = t) > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = MUL;  }
      *++e = ADD;
    }
    else if (tk == Sub) {
      next(); *++e = PSH; expr(Mul);
      if (t > PTR && t == ty) { *++e = SUB; *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = DIV; ty = INT; }
      else if ((ty = t) > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = MUL; *++e = SUB; }
      else *++e = SUB;
    }
    else if (tk == Mul) { next(); *++e = PSH; expr(Inc); *++e = MUL; ty = INT; }
    else if (tk == Div) { next(); *++e = PSH; expr(Inc); *++e = DIV; ty = INT; }
    else if (tk == Mod) { next(); *++e = PSH; expr(Inc); *++e = MOD; ty = INT; }
    else if (tk == Inc || tk == Dec) {
      if (*e == LC) { *e = PSH; *++e = LC; }
      else if (*e == LI) { *e = PSH; *++e = LI; }
      else { printf("%d: bad lvalue in post-increment\n", line); exit(-1); }
      *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(int) : sizeof(char);
      *++e = (tk == Inc) ? ADD : SUB;
      *++e = (ty == CHAR) ? SC : SI;
      *++e = PSH; *++e = IMM; *++e = (ty > PTR) ? sizeof(int) : sizeof(char);
      *++e = (tk == Inc) ? SUB : ADD;
      next();
    }
    else if (tk == Brak) {
      next(); *++e = PSH; expr(Assign);
      if (tk == ']') next(); else { printf("%d: close bracket expected\n", line); exit(-1); }
      if (t > PTR) { *++e = PSH; *++e = IMM; *++e = sizeof(int); *++e = MUL;  }
      else if (t < PTR) { printf("%d: pointer type expected\n", line); exit(-1); }
      *++e = ADD;
      *++e = ((ty = t - PTR) == CHAR) ? LC : LI;
    }
    else { printf("%d: compiler error tk=%d\n", line, tk); exit(-1); }
  }
}

/*
	stmt() -- Parse statements
	Parses and generates code for the statements
*/
void stmt()
{
  int *a, *b; //Temporary pointer for branches

  if (tk == If) { //If token is if statement
    next(); //Go to next token
    if (tk == '(') next(); else { printf("%d: open paren expected\n", line); exit(-1); } //Ensure starts with (
    expr(Assign); //Evaluate Condition
    if (tk == ')') next(); else { printf("%d: close paren expected\n", line); exit(-1); } //Ensure ends with )
    *++e = BZ; b = ++e; //Emit branch instruction if false
    stmt(); //Parse if body
    if (tk == Else) { //Handle else
      *b = (int)(e + 3); *++e = JMP; b = ++e; //Patch if branch and jump to else
      next(); //Get next token
      stmt(); //Parse else body
    }
    *b = (int)(e + 1); //Patch the jump target
  }
  else if (tk == While) { //If token is while
    next(); //Go to next token
    a = e + 1; //Store loop starting address
    if (tk == '(') next(); else { printf("%d: open paren expected\n", line); exit(-1); } //Ensure starts with (
    expr(Assign); //Check loop condition
    if (tk == ')') next(); else { printf("%d: close paren expected\n", line); exit(-1); } //Ensure ends with )
    *++e = BZ; b = ++e; //Emit branch if conditioon is false
    stmt(); //Parse loop body
    *++e = JMP; *++e = (int)a; //Emit jump back to the start of loop
    *b = (int)(e + 1); //Patch exit jump
  }
  else if (tk == Return) { //If token is return
    next(); //Go to next token
    if (tk != ';') expr(Assign); //Evaluate return expression
    *++e = LEV; //Emit return instruction
    if (tk == ';') next(); else { printf("%d: semicolon expected\n", line); exit(-1); } //Ensure ; at end
  }
  else if (tk == '{') { //if toke {
    next(); //Go to next token
    while (tk != '}') stmt(); // loop until } and parse inside statements
    next(); //Go to next token
  }
  else if (tk == ';') { //if token is ;
    next(); //Go to next token
  }
  else { //If expression
    expr(Assign);
    if (tk == ';') next(); else { printf("%d: semicolon expected\n", line); exit(-1); }//Ensure ; at end
  }
}

int main(int argc, char **argv)
{
  int fd, bt, ty, poolsz, *idmain;
  int *pc, *sp, *bp, a, cycle; // vm registers
  int i, *t; // temps

  --argc; ++argv; //Skip program name
  if (argc > 0 && **argv == '-' && (*argv)[1] == 's') { src = 1; --argc; ++argv; } //Enable source debugging
  if (argc > 0 && **argv == '-' && (*argv)[1] == 'd') { debug = 1; --argc; ++argv; } //Enable execution debugging
  if (argc < 1) { printf("usage: c4 [-s] [-d] file ...\n"); return -1; } //Print usage if no input file

  if ((fd = open(*argv, 0)) < 0) { printf("could not open(%s)\n", *argv); return -1; } //Open source file

  poolsz = 256*1024; // arbitrary size
  //Allocate symbol, text, data, and stack
  if (!(sym = malloc(poolsz))) { printf("could not malloc(%d) symbol area\n", poolsz); return -1; }
  if (!(le = e = malloc(poolsz))) { printf("could not malloc(%d) text area\n", poolsz); return -1; }
  if (!(data = malloc(poolsz))) { printf("could not malloc(%d) data area\n", poolsz); return -1; }
  if (!(sp = malloc(poolsz))) { printf("could not malloc(%d) stack area\n", poolsz); return -1; }

  //Initialize symbol, text, data to 0
  memset(sym,  0, poolsz);
  memset(e,    0, poolsz);
  memset(data, 0, poolsz);

  //Define keywords and standard library functions
  p = "char else enum if int return sizeof while "
      "open read close printf malloc free memset memcmp exit void main";
  i = Char; while (i <= While) { next(); id[Tk] = i++; } // add keywords to symbol table
  i = OPEN; while (i <= EXIT) { next(); id[Class] = Sys; id[Type] = INT; id[Val] = i++; } // add library to symbol table
  next(); id[Tk] = Char; // handle void type
  next(); idmain = id; // keep track of main

  if (!(lp = p = malloc(poolsz))) { printf("could not malloc(%d) source area\n", poolsz); return -1; } //Allocate source buffer
  if ((i = read(fd, p, poolsz-1)) <= 0) { printf("read() returned %d\n", i); return -1; } //Read source file into memory
  p[i] = 0; //Null terminate source code
  close(fd); //Close source file

  // parse declarations
  line = 1;
  next();
  while (tk) {
    bt = INT; // basetype
    if (tk == Int) next();
    else if (tk == Char) { next(); bt = CHAR; }
    else if (tk == Enum) { //Handle enum definitions
      next();
      if (tk != '{') next();
      if (tk == '{') {
        next();
        i = 0;
        while (tk != '}') { //Process enum values
          if (tk != Id) { printf("%d: bad enum identifier %d\n", line, tk); return -1; }
          next();
          if (tk == Assign) {
            next();
            if (tk != Num) { printf("%d: bad enum initializer\n", line); return -1; }
            i = ival;
            next();
          }
          id[Class] = Num; id[Type] = INT; id[Val] = i++;
          if (tk == ',') next();
        }
        next();
      }
    }
    while (tk != ';' && tk != '}') { //Handle the global variables or functions
      ty = bt;
      while (tk == Mul) { next(); ty = ty + PTR; } //handle pointer types
      if (tk != Id) { printf("%d: bad global declaration\n", line); return -1; }
      if (id[Class]) { printf("%d: duplicate global definition\n", line); return -1; }
      next();
      id[Type] = ty;
      if (tk == '(') { //Function declaration
        id[Class] = Fun;
        id[Val] = (int)(e + 1);
        next(); i = 0;
        while (tk != ')') { //Parse function parameters
          ty = INT;
          if (tk == Int) next();
          else if (tk == Char) { next(); ty = CHAR; }
          while (tk == Mul) { next(); ty = ty + PTR; }
          if (tk != Id) { printf("%d: bad parameter declaration\n", line); return -1; }
          if (id[Class] == Loc) { printf("%d: duplicate parameter definition\n", line); return -1; }
          id[HClass] = id[Class]; id[Class] = Loc;
          id[HType]  = id[Type];  id[Type] = ty;
          id[HVal]   = id[Val];   id[Val] = i++;
          next();
          if (tk == ',') next();
        }
        next();
        if (tk != '{') { printf("%d: bad function definition\n", line); return -1; }
        loc = ++i;
        next();
        while (tk == Int || tk == Char) { //Parse local variables
          bt = (tk == Int) ? INT : CHAR;
          next();
          while (tk != ';') {
            ty = bt;
            while (tk == Mul) { next(); ty = ty + PTR; }
            if (tk != Id) { printf("%d: bad local declaration\n", line); return -1; }
            if (id[Class] == Loc) { printf("%d: duplicate local definition\n", line); return -1; }
            id[HClass] = id[Class]; id[Class] = Loc;
            id[HType]  = id[Type];  id[Type] = ty;
            id[HVal]   = id[Val];   id[Val] = ++i;
            next();
            if (tk == ',') next();
          }
          next();
        }
        *++e = ENT; *++e = i - loc; //Enit function prologue
        while (tk != '}') stmt(); //Parse function body
        *++e = LEV; //Emit function epilogue
        id = sym; // unwind symbol table locals
        while (id[Tk]) {
          if (id[Class] == Loc) {
            id[Class] = id[HClass];
            id[Type] = id[HType];
            id[Val] = id[HVal];
          }
          id = id + Idsz;
        }
      }
      else { //Global variable declaration
        id[Class] = Glo;
        id[Val] = (int)data;
        data = data + sizeof(int);
      }
      if (tk == ',') next();
    }
    next();
  }

  if (!(pc = (int *)idmain[Val])) { printf("main() not defined\n"); return -1; }
  if (src) return 0; //If source debugging, exit before execution

  // setup stack
  bp = sp = (int *)((int)sp + poolsz);
  *--sp = EXIT; // call exit if main returns
  *--sp = PSH; t = sp;
  *--sp = argc;
  *--sp = (int)argv;
  *--sp = (int)t;

  //Run compiled program
  cycle = 0;
  while (1) {
    i = *pc++; ++cycle;
    if (debug) { //Print debug info
      printf("%d> %.4s", cycle,
        &"LEA ,IMM ,JMP ,JSR ,BZ  ,BNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,"
         "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
         "OPEN,READ,CLOS,PRTF,MALC,FREE,MSET,MCMP,EXIT,"[i * 5]);
      if (i <= ADJ) printf(" %d\n", *pc); else printf("\n");
    }
    if      (i == LEA) a = (int)(bp + *pc++);                             // load local address
    else if (i == IMM) a = *pc++;                                         // load global address or immediate
    else if (i == JMP) pc = (int *)*pc;                                   // jump
    else if (i == JSR) { *--sp = (int)(pc + 1); pc = (int *)*pc; }        // jump to subroutine
    else if (i == BZ)  pc = a ? pc + 1 : (int *)*pc;                      // branch if zero
    else if (i == BNZ) pc = a ? (int *)*pc : pc + 1;                      // branch if not zero
    else if (i == ENT) { *--sp = (int)bp; bp = sp; sp = sp - *pc++; }     // enter subroutine
    else if (i == ADJ) sp = sp + *pc++;                                   // stack adjust
    else if (i == LEV) { sp = bp; bp = (int *)*sp++; pc = (int *)*sp++; } // leave subroutine
    else if (i == LI)  a = *(int *)a;                                     // load int
    else if (i == LC)  a = *(char *)a;                                    // load char
    else if (i == SI)  *(int *)*sp++ = a;                                 // store int
    else if (i == SC)  a = *(char *)*sp++ = a;                            // store char
    else if (i == PSH) *--sp = a;                                         // push

    else if (i == OR)  a = *sp++ |  a;
    else if (i == XOR) a = *sp++ ^  a;
    else if (i == AND) a = *sp++ &  a;
    else if (i == EQ)  a = *sp++ == a;
    else if (i == NE)  a = *sp++ != a;
    else if (i == LT)  a = *sp++ <  a;
    else if (i == GT)  a = *sp++ >  a;
    else if (i == LE)  a = *sp++ <= a;
    else if (i == GE)  a = *sp++ >= a;
    else if (i == SHL) a = *sp++ << a;
    else if (i == SHR) a = *sp++ >> a;
    else if (i == ADD) a = *sp++ +  a;
    else if (i == SUB) a = *sp++ -  a;
    else if (i == MUL) a = *sp++ *  a;
    else if (i == DIV) a = *sp++ /  a;
    else if (i == MOD) a = *sp++ %  a;

    else if (i == OPEN) a = open((char *)sp[1], *sp);
    else if (i == READ) a = read(sp[2], (char *)sp[1], *sp);
    else if (i == CLOS) a = close(*sp);
    else if (i == PRTF) { t = sp + pc[1]; a = printf((char *)t[-1], t[-2], t[-3], t[-4], t[-5], t[-6]); }
    else if (i == MALC) a = (int)malloc(*sp);
    else if (i == FREE) free((void *)*sp);
    else if (i == MSET) a = (int)memset((char *)sp[2], sp[1], *sp);
    else if (i == MCMP) a = memcmp((char *)sp[2], (char *)sp[1], *sp);
    else if (i == EXIT) { printf("exit(%d) cycle = %d\n", *sp, cycle); return *sp; }
    else { printf("unknown instruction = %d! cycle = %d\n", i, cycle); return -1; }
  }
}