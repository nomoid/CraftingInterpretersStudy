#ifndef clox_compiler_h
#define clox_compiler_h

#include "object.h"
#include "vm.h"
#include "scanner.h"
#include "table.h"
#include "bitfield.h"
#include "common.h"

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
    Scanner* scanner;
    Chunk* currentChunk;
} Parser;

typedef struct {
    Token name;
    int depth;
#ifdef CLOX_CONST_KEYWORD
    bool constant;
#endif
} Local;

#define DEFAULT_LOCAL_COUNT UINT8_COUNT

typedef struct Compiler {
    Local locals[DEFAULT_LOCAL_COUNT];
    int localCount;
    int scopeDepth;
    Parser parser;
    FreeList* freeList;
    Table* strings;
} Compiler;

typedef void (*ParseFn)(Compiler *, bool);

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

bool compile(VM* vm, const char* source, Chunk* chunk);

#endif