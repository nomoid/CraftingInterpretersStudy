#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"
#include "common.h"
#include "compiler.h"
#include "endian.h"
#include "memory.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

static void expression(Compiler* compiler);
static void statement(Compiler* compiler);
static void declaration(Compiler* compiler);
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Compiler* compiler, Precedence precedence);

static void errorAt(Compiler* compiler, Token* token, const char* message) {
    if (compiler->parser.panicMode) {
        return;
    }
    compiler->parser.panicMode = true;
    fprintf(stderr, "[line %" FORMAT_SIZE_T "] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR) {
        // Do nothing
    }
    else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    compiler->parser.hadError = true;
}

static void errorAtCurrent(Compiler* compiler, const char* message) {
    errorAt(compiler, &compiler->parser.current, message);
}

static void error(Compiler* compiler, const char* message) {
    errorAt(compiler, &compiler->parser.previous, message);
}

static void advance(Compiler* compiler) {
    compiler->parser.previous = compiler->parser.current;

    while (1) {
        scanToken(compiler->parser.scanner, &compiler->parser.current);
        if (compiler->parser.current.type != TOKEN_ERROR) {
            break;
        }

        errorAtCurrent(compiler, compiler->parser.current.start);
    }
}

static void consume(Compiler* compiler,
        TokenType type, const char* message) {
    if (compiler->parser.current.type == type) {
        advance(compiler);
        return;
    }

    errorAtCurrent(compiler, message);
}

static bool check(Compiler* compiler, TokenType type) {
    return compiler->parser.current.type == type;
}

static bool match(Compiler* compiler, TokenType type) {
    if (!check(compiler, type)) {
        return false;
    }
    advance(compiler);
    return true;
}

static Chunk* currentChunk(Compiler* compiler) {
    return compiler->parser.currentChunk;
}

static void emitByte(Compiler* compiler, uint8_t byte) {
    int result = writeChunk(
        currentChunk(compiler), byte, compiler->parser.previous.line);
    if (result < 0) {
        error(compiler, "Failed chunk allocation.");
    }
}

static void emitBytes(Compiler* compiler, uint8_t byte1, uint8_t byte2) {
    emitByte(compiler, byte1);
    emitByte(compiler, byte2);
}

#if defined(CLOX_LONG_CONSTANTS) || defined(CLOX_LONG_LOCALS)
// Remainder is expected to be 3-bytes-long
static void emitBytesLong(Compiler* compiler, uint8_t byte1, size_t remainder) {
    emitByte(compiler, byte1);
    for (int i = 0; i < 3; i++) {
        emitByte(compiler, BYTE_FROM_3WORD(remainder, i));
    }
}
#endif

static void emitReturn(Compiler* compiler) {
    emitByte(compiler, OP_RETURN);
}

static void endCompiler(Compiler* compiler) {
    emitReturn(compiler);
#ifdef DEBUG_PRINT_CODE
    if (!compiler->parser.hadError) {
        disassembleChunk(currentChunk(parser), "code");
    }
#endif
}

static void beginScope(Compiler* compiler) {
    compiler->scopeDepth++;
}

static void endScope(Compiler* compiler) {
    compiler->scopeDepth--;

    while (compiler->localCount > 0 &&
            compiler->locals[compiler->localCount - 1].depth >
            compiler->scopeDepth) {
        emitByte(compiler, OP_POP);
        compiler->localCount--;
    }
}

static void emitConstant(Compiler* compiler, Value value) {
    int constant = writeConstant(
        currentChunk(compiler), value, compiler->parser.previous.line);
    if (constant < 0) {
        error(compiler, "Too many constants in one chunk.");
    }
}

static size_t identifierConstant(Compiler* compiler, Token* name) {
    int result = addConstant(
        compiler->parser.currentChunk,
        OBJ_VAL(copyString(
            compiler->freeList, compiler->strings, name->start, name->length))
    );
    if (result < 0) {
        error(compiler, "Too many constants in one chunk.");
    }
    return (size_t) result;
}

static bool identifiersEqual(Token* a, Token* b) {
    return a->length == b->length &&
        memcmp(a->start, b->start, (size_t) a->length) == 0;
}

static size_t resolveLocal(Compiler* compiler, Token* name, bool* isConst) {
    for (int i = (int)(compiler->localCount - 1); i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name)) {
            if (local->depth == -1) {
                error(compiler,
                    "Cannot read local variable in its own initializer");
            }
#ifdef CLOX_CONST_KEYWORD
            *isConst = local->constant;
#else
            *isConst = false;
#endif
            return (size_t) i;
        }
    }

    return (size_t) -1;
}

static void addLocal(Compiler* compiler, Token name, bool constDecl) {
    if (compiler->localCount == MAX_LOCAL_COUNT) {
        error(compiler, "Too many local variables in function.");
        return;
    }
#ifdef CLOX_LONG_LOCALS
    if (compiler->localCount == compiler->localCapacity) {
        size_t newCapacity = GROW_CAPACITY(compiler->localCapacity);
        compiler->locals = GROW_ARRAY(
            compiler->locals,
            Local,
            compiler->localCapacity,
            newCapacity);
        compiler->localCapacity = newCapacity;
    }
#endif
    Local* local = &compiler->locals[compiler->localCount++];
    local->name = name;
    local->depth = -1;
#ifdef CLOX_CONST_KEYWORD
    local->constant = constDecl;
#else
    UNUSED(constDecl);
#endif
}

static void declareVariable(Compiler* compiler, bool constDecl) {
    if (compiler->scopeDepth == 0) {
        return;
    }

    Token* name = &compiler->parser.previous;
    for (int i = (int)(compiler->localCount - 1); i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (local->depth != -1 && local->depth < compiler->scopeDepth) {
            break;
        }
        if (identifiersEqual(name, &local->name)) {
            error(compiler,
                "Variable with this name already declared in this scope.");
        }
    }
    addLocal(compiler, *name, constDecl);
}

static size_t parseVariable(Compiler* compiler, bool constDecl,
        const char* errorMessage) {
    consume(compiler, TOKEN_IDENTIFIER, errorMessage);

    declareVariable(compiler, constDecl);
    if (compiler->scopeDepth > 0) {
        return 0;
    }

    return identifierConstant(compiler, &compiler->parser.previous);
}

static void markInitialized(Compiler* compiler) {
    compiler->locals[compiler->localCount - 1].depth =
        compiler->scopeDepth;
}

static void defineVariable(Compiler* compiler, size_t global, bool constDecl) {
    if (compiler->scopeDepth > 0) {
        markInitialized(compiler);
        return;
    }
    #ifdef CLOX_LONG_CONSTANTS
    if (global > CHUNK_SHORT_CONSTANTS) {
        uint8_t opcode;
        if (constDecl) {
            opcode = OP_DEFINE_GLOBAL_CONST_LONG;
        }
        else {
            opcode = OP_DEFINE_GLOBAL_LONG;
        }
        emitBytesLong(compiler, opcode, global);
    }
    #else
    if (global > CHUNK_SHORT_CONSTANTS) {
        error(compiler, "Too many constants.");
    }
    #endif
    else {
        uint8_t opcode;
        if (constDecl) {
            opcode = OP_DEFINE_GLOBAL_CONST;
        }
        else {
            opcode = OP_DEFINE_GLOBAL;
        }
        emitBytes(compiler, opcode, (uint8_t) global);
    }
}


static void number(Compiler* compiler, bool canAssign) {
    UNUSED(canAssign);
    TokenType numberType = compiler->parser.previous.type;
    Value value;
    if (numberType == TOKEN_NUMBER) {
        double vfloat = strtod(compiler->parser.previous.start, NULL);
        value = FLOAT_VAL(vfloat);
    }
#ifdef CLOX_INTEGER_TYPE
    else if (numberType == TOKEN_INTEGER) {
        int64_t vint = strtoll(compiler->parser.previous.start, NULL, 10);
        value = INT_VAL(vint);
    }
#endif
    else {
        error(compiler, "Illegal number type.");
        return;
    }
    emitConstant(compiler, value);
}

static void string(Compiler* compiler, bool canAssign) {
    UNUSED(canAssign);
    emitConstant(compiler, OBJ_VAL(copyString(
        compiler->freeList,
        compiler->strings,
        compiler->parser.previous.start + 1,
        compiler->parser.previous.length - 2)));
}

static void namedVariable(Compiler* compiler, Token name, bool canAssign) {
    bool isConst = false;
    size_t arg = resolveLocal(compiler, &name, &isConst);
    uint8_t setOp;
    uint8_t getOp;
    bool shortOp = true;
    if (arg != (size_t) -1) {
#ifdef CLOX_LONG_LOCALS
        if (arg >= DEFAULT_LOCAL_COUNT) {
            shortOp = false;
            setOp = OP_SET_LOCAL_LONG;
            getOp = OP_GET_LOCAL_LONG;
        }
#else
        if (arg > DEFAULT_LOCAL_COUNT) {
            error(compiler, "Too many local variables.");
            return;
        }
#endif
        else {    
            setOp = OP_SET_LOCAL;
            getOp = OP_GET_LOCAL;
        }
    }
    else {
        arg = identifierConstant(compiler, &name);
#ifdef CLOX_LONG_CONSTANTS
        if (arg > CHUNK_SHORT_CONSTANTS) {
            shortOp = false;
            setOp = OP_SET_GLOBAL_LONG;
            getOp = OP_GET_GLOBAL_LONG;
        }
#else
        if (arg > CHUNK_SHORT_CONSTANTS) {
            error(compiler, "Too many constants.");
            return;
        }
#endif
        else {
            setOp = OP_SET_GLOBAL;
            getOp = OP_GET_GLOBAL;
        }
    }
    if (shortOp) {
        if (canAssign && match(compiler, TOKEN_EQUAL)) {
            expression(compiler);
            if (isConst) {
                error(compiler,
                    "Cannot overwrite the value of a local const.");
                return;
            }
            emitBytes(compiler, setOp, (uint8_t) arg);
        }
        else {
            emitBytes(compiler, getOp, (uint8_t) arg);
        }
    }
    else {
        if (canAssign && match(compiler, TOKEN_EQUAL)) {
            expression(compiler);
            if (isConst) {
                error(compiler,
                    "Cannot overwrite the value of a local const.");
                return;
            }
            emitBytesLong(compiler, setOp, arg);
        }
        else {
            emitBytesLong(compiler, getOp, arg);
        }
    }
}

static void variable(Compiler* compiler, bool canAssign) {
    namedVariable(compiler, compiler->parser.previous, canAssign);
}

static void grouping(Compiler* compiler, bool canAssign) {
    UNUSED(canAssign);
    expression(compiler);
    consume(compiler, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void binary(Compiler* compiler, bool canAssign) {
    UNUSED(canAssign);
    // Remember the operator
    TokenType operatorType = compiler->parser.previous.type;

    ParseRule* rule = getRule(operatorType);
    parsePrecedence(compiler, (Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:    emitBytes(compiler, OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:   emitByte(compiler, OP_EQUAL); break;
        case TOKEN_GREATER:       emitByte(compiler, OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(compiler, OP_LESS, OP_NOT); break;
        case TOKEN_LESS:          emitByte(compiler, OP_LESS); break;
        case TOKEN_LESS_EQUAL:    emitBytes(compiler, OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:          emitByte(compiler, OP_ADD); break;
        case TOKEN_MINUS:         emitByte(compiler, OP_SUBTRACT); break;
        case TOKEN_STAR:          emitByte(compiler, OP_MULTIPLY); break;
        case TOKEN_SLASH:         emitByte(compiler, OP_DIVIDE); break;
        default:
            return; // Unreachable.
    }
}

static void literal(Compiler* compiler, bool canAssign) {
    UNUSED(canAssign);
    switch (compiler->parser.previous.type) {
        case TOKEN_FALSE: emitByte(compiler, OP_FALSE); break;
        case TOKEN_TRUE: emitByte(compiler, OP_TRUE); break;
        case TOKEN_NIL: emitByte(compiler, OP_NIL); break;
        default:
            return; // Unreachable.
    }
}

static void unary(Compiler* compiler, bool canAssign) {
    UNUSED(canAssign);

    TokenType operatorType = compiler->parser.previous.type;

    // Compile the operand
    parsePrecedence(compiler, PREC_UNARY);

    // Emit the operator instrution
    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(compiler, OP_NEGATE);
            break;
        case TOKEN_BANG:
            emitByte(compiler, OP_NOT);
            break;
        default:
            // Unreachable code
            return;
    }
}

ParseRule rules[] = {
  { grouping, NULL,    PREC_NONE },       // TOKEN_LEFT_PAREN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_PAREN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_LEFT_BRACE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RIGHT_BRACE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_COMMA
  { NULL,     NULL,    PREC_NONE },       // TOKEN_DOT
  { unary,    binary,  PREC_TERM },       // TOKEN_MINUS
  { NULL,     binary,  PREC_TERM },       // TOKEN_PLUS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SEMICOLON
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_SLASH
  { NULL,     binary,  PREC_FACTOR },     // TOKEN_STAR
  { unary,    NULL,    PREC_NONE },       // TOKEN_BANG
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_BANG_EQUAL
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EQUAL
  { NULL,     binary,  PREC_EQUALITY },   // TOKEN_EQUAL_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_GREATER_EQUAL
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS
  { NULL,     binary,  PREC_COMPARISON }, // TOKEN_LESS_EQUAL
  { variable, NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
  { string,   NULL,    PREC_NONE },       // TOKEN_STRING
  { number,   NULL,    PREC_NONE },       // TOKEN_NUMBER
#ifdef CLOX_INTEGER_TYPE
  { number,   NULL,    PREC_NONE },       // TOKEN_INTEGER
#endif
  { NULL,     NULL,    PREC_NONE },       // TOKEN_AND
  { NULL,     NULL,    PREC_NONE },       // TOKEN_CLASS
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ELSE
  { literal,  NULL,    PREC_NONE },       // TOKEN_FALSE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FOR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_FUN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IF
  { literal,  NULL,    PREC_NONE },       // TOKEN_NIL
  { NULL,     NULL,    PREC_NONE },       // TOKEN_OR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_PRINT
  { NULL,     NULL,    PREC_NONE },       // TOKEN_RETURN
  { NULL,     NULL,    PREC_NONE },       // TOKEN_SUPER
  { NULL,     NULL,    PREC_NONE },       // TOKEN_THIS
  { literal,  NULL,    PREC_NONE },       // TOKEN_TRUE
  { NULL,     NULL,    PREC_NONE },       // TOKEN_VAR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_WHILE
#ifdef CLOX_CONST_KEYWORD
  { NULL,     NULL,    PREC_NONE },       // TOKEN_CONST
#endif
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EOF
};

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void parsePrecedence(Compiler* compiler, Precedence precedence) {
    advance(compiler);
    ParseFn prefixRule = getRule(compiler->parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error(compiler, "Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(compiler, canAssign);

    while (precedence <= getRule(compiler->parser.current.type)->precedence) {
        advance(compiler);
        ParseFn infixRule = getRule(compiler->parser.previous.type)->infix;
        infixRule(compiler, canAssign);
    }

    if (canAssign && match(compiler, TOKEN_EQUAL)) {
        error(compiler, "Invalid assigment target.");
    }
}

static void expression(Compiler* compiler) {
    parsePrecedence(compiler, PREC_ASSIGNMENT);
}

static void block(Compiler* compiler) {
    while (!check(compiler, TOKEN_RIGHT_BRACE)
            && !check(compiler, TOKEN_EOF)) {
        declaration(compiler);
    }

    consume(compiler, TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void varDeclaration(Compiler* compiler, bool constDecl) {
    size_t global = parseVariable(compiler, constDecl,"Expect variable name.");

    if (match(compiler, TOKEN_EQUAL)) {
        expression(compiler);
    }
    else {
        emitByte(compiler, OP_NIL);
    }
    consume(compiler, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(compiler, global, constDecl);
}

static void printStatement(Compiler* compiler) {
    expression(compiler);
    consume(compiler, TOKEN_SEMICOLON, "Expect ';' after value for print.");
    emitByte(compiler, OP_PRINT);
}

static void synchronize(Compiler* compiler) {
    compiler->parser.panicMode = false;

    while (compiler->parser.current.type != TOKEN_EOF) {
        if (compiler->parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }

        switch (compiler->parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                // do nothing
                ;
        }

        advance(compiler);
    }
}

static void expressionStatement(Compiler* compiler) {
    expression(compiler);
    consume(compiler, TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(compiler, OP_POP);
}

static void statement(Compiler* compiler) {
    if (match(compiler, TOKEN_PRINT)) {
        printStatement(compiler);
    }
    else if (match(compiler, TOKEN_LEFT_BRACE)) {
        beginScope(compiler);
        block(compiler);
        endScope(compiler);
    }
    else {
        expressionStatement(compiler);
    }
}

static void declaration(Compiler* compiler) {
    if (match(compiler, TOKEN_VAR)) {
        varDeclaration(compiler, false);
    }
#ifdef CLOX_CONST_KEYWORD
    else if (match(compiler, TOKEN_CONST)) {
        varDeclaration(compiler, true);
    }
#endif
    else {
        statement(compiler);
    }

    if (compiler->parser.panicMode) {
        synchronize(compiler);
    }
}

static void initCompiler(Compiler* compiler) {
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
#ifdef CLOX_LONG_LOCALS
    compiler->localCapacity = DEFAULT_LOCAL_COUNT;
    compiler->locals = ALLOCATE(Local, compiler->localCapacity, false);
#endif
}

static void freeCompiler(Compiler* compiler) {
#ifdef CLOX_LONG_LOCALS
    FREE_ARRAY(Local, compiler->locals, compiler->localCapacity);
    compiler->locals = NULL;
#else
    UNUSED(compiler);
#endif
}

bool compile(VM* vm, const char* source, Chunk* chunk) {
    Scanner scanner;
    initScanner(&scanner, source);

    Compiler compiler;
    initCompiler(&compiler);

    Parser* parser = &compiler.parser;
    parser->scanner = &scanner;
    parser->currentChunk = chunk;
    parser->hadError = false;
    parser->panicMode = false;
    parser->previous.type = TOKEN_ERROR;
    parser->current.type = TOKEN_ERROR;

    compiler.strings = &vm->strings;
    compiler.freeList = &vm->freeList;

    advance(&compiler);

    while(!match(&compiler, TOKEN_EOF)) {
        declaration(&compiler);
    }

    endCompiler(&compiler);
    freeCompiler(&compiler);

    return !compiler.parser.hadError;
}