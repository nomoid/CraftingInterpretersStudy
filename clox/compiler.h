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
#ifdef CLOX_LONG_LOCALS
    // 2**24 - 2
    #define MAX_LOCAL_COUNT 16777214
#else
    #define MAX_LOCAL_COUNT DEFAULT_LOCAL_COUNT
#endif

typedef struct Compiler {
#ifdef CLOX_LONG_LOCALS
    Local* locals;
#else
    Local locals[DEFAULT_LOCAL_COUNT];
#endif
    size_t localCount;
    int scopeDepth;
    Parser parser;
    FreeList* freeList;
    Table* strings;
#ifdef CLOX_LONG_LOCALS
    size_t localCapacity;
#endif
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