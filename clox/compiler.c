#include <stdio.h>
#include <stdlib.h>

#include "settings.h"
#include "common.h"
#include "compiler.h"
#include "endian.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

static void expression(Parser* parser);
static void statement(Parser* parser);
static void declaration(Parser* parser);
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Parser* parser, Precedence precedence);

static void errorAt(Parser* parser, Token* token, const char* message) {
    if (parser->panicMode) {
        return;
    }
    parser->panicMode = true;
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
    parser->hadError = true;
}

static void errorAtCurrent(Parser* parser, const char* message) {
    errorAt(parser, &parser->current, message);
}

static void error(Parser* parser, const char* message) {
    errorAt(parser, &parser->previous, message);
}

static void advance(Parser* parser) {
    parser->previous = parser->current;

    while (1) {
        scanToken(parser->scanner, &parser->current);
        if (parser->current.type != TOKEN_ERROR) {
            break;
        }

        errorAtCurrent(parser, parser->current.start);
    }
}

static void consume(Parser* parser,
        TokenType type, const char* message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }

    errorAtCurrent(parser, message);
}

static bool check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

static bool match(Parser* parser, TokenType type) {
    if (!check(parser, type)) {
        return false;
    }
    advance(parser);
    return true;
}

static Chunk* currentChunk(Parser* parser) {
    return parser->currentChunk;
}

static void emitByte(Parser* parser, uint8_t byte) {
    int result = writeChunk(
        currentChunk(parser), byte, parser->previous.line);
    if (result < 0) {
        error(parser, "Failed chunk allocation.");
    }
}

static void emitBytes(Parser* parser, uint8_t byte1, uint8_t byte2) {
    emitByte(parser, byte1);
    emitByte(parser, byte2);
}

// Remainder is expected to be 3-bytes-long
static void emitBytesLong(Parser* parser, uint8_t byte1, size_t remainder) {
    emitByte(parser, byte1);
    for (int i = 0; i < 3; i++) {
        emitByte(parser, BYTE_FROM_3WORD(remainder, i));
    }
}

static void emitReturn(Parser* parser) {
    emitByte(parser, OP_RETURN);
}

static void endCompiler(Parser* parser) {
    emitReturn(parser);
#ifdef DEBUG_PRINT_CODE
    if (!parser->hadError) {
        disassembleChunk(currentChunk(parser), "code");
    }
#endif
}

static void emitConstant(Parser* parser, Value value) {
    int constant = writeConstant(
        currentChunk(parser), value, parser->previous.line);
    if (constant < 0) {
        error(parser, "Too many constants in one chunk.");
    }
}

static size_t identifierConstant(Parser* parser, Token* name) {
    int result = addConstant(
        parser->currentChunk,
        OBJ_VAL(copyString(
            &parser->freeList, &parser->strings, name->start, name->length))
    );
    if (result < 0) {
        error(parser, "Too many constants in one chunk.");
    }
    return (size_t) result;
}

static size_t parseVariable(Parser* parser, const char* errorMessage) {
    consume(parser, TOKEN_IDENTIFIER, errorMessage);
    return identifierConstant(parser, &parser->previous);
}

static void defineVariable(Parser* parser, size_t global) {
    #ifdef CLOX_LONG_CONSTANTS
    if (global > CHUNK_SHORT_CONSTANTS) {
        emitBytesLong(parser, OP_DEFINE_GLOBAL_LONG, global);
    }
    #else
    if (global > CHUNK_SHORT_CONSTANTS) {
        error(parser, "Too many constants.");
    }
    #endif
    else {
        emitBytes(parser, OP_DEFINE_GLOBAL, (uint8_t) global);
    }
}


static void number(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    TokenType numberType = parser->previous.type;
    Value value;
    if (numberType == TOKEN_NUMBER) {
        double vfloat = strtod(parser->previous.start, NULL);
        value = FLOAT_VAL(vfloat);
    }
#ifdef CLOX_INTEGER_TYPE
    else if (numberType == TOKEN_INTEGER) {
        int64_t vint = strtoll(parser->previous.start, NULL, 10);
        value = INT_VAL(vint);
    }
#endif
    else {
        error(parser, "Illegal number type.");
        return;
    }
    emitConstant(parser, value);
}

static void string(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    emitConstant(parser, OBJ_VAL(copyString(
        &parser->freeList,
        &parser->strings,
        parser->previous.start + 1,
        parser->previous.length - 2)));
}

static void namedVariable(Parser* parser, Token name, bool canAssign) {
    size_t arg = identifierConstant(parser, &name);
    #ifdef CLOX_LONG_CONSTANTS
    if (arg > CHUNK_SHORT_CONSTANTS) {
        if (canAssign && match(parser, TOKEN_EQUAL)) {
            expression(parser);
            emitBytesLong(parser, OP_SET_GLOBAL_LONG, arg);
        }
        else {
            emitBytesLong(parser, OP_GET_GLOBAL_LONG, arg);
        }
    }
    #else
    if (global > CHUNK_SHORT_CONSTANTS) {
        error(parser, "Too many constants.");
    }
    #endif
    else {
        if (canAssign && match(parser, TOKEN_EQUAL)) {
            expression(parser);
            emitBytes(parser, OP_SET_GLOBAL, (uint8_t) arg);
        }
        else {
            emitBytes(parser, OP_GET_GLOBAL, (uint8_t) arg);
        }
    }
}

static void variable(Parser* parser, bool canAssign) {
    namedVariable(parser, parser->previous, canAssign);
}

static void grouping(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void binary(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    // Remember the operator
    TokenType operatorType = parser->previous.type;

    ParseRule* rule = getRule(operatorType);
    parsePrecedence(parser, (Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL:    emitBytes(parser, OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:   emitByte(parser, OP_EQUAL); break;
        case TOKEN_GREATER:       emitByte(parser, OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(parser, OP_LESS, OP_NOT); break;
        case TOKEN_LESS:          emitByte(parser, OP_LESS); break;
        case TOKEN_LESS_EQUAL:    emitBytes(parser, OP_GREATER, OP_NOT); break;
        case TOKEN_PLUS:          emitByte(parser, OP_ADD); break;
        case TOKEN_MINUS:         emitByte(parser, OP_SUBTRACT); break;
        case TOKEN_STAR:          emitByte(parser, OP_MULTIPLY); break;
        case TOKEN_SLASH:         emitByte(parser, OP_DIVIDE); break;
        default:
            return; // Unreachable.
    }
}

static void literal(Parser* parser, bool canAssign) {
    UNUSED(canAssign);
    switch (parser->previous.type) {
        case TOKEN_FALSE: emitByte(parser, OP_FALSE); break;
        case TOKEN_TRUE: emitByte(parser, OP_TRUE); break;
        case TOKEN_NIL: emitByte(parser, OP_NIL); break;
        default:
            return; // Unreachable.
    }
}

static void unary(Parser* parser, bool canAssign) {
    UNUSED(canAssign);

    TokenType operatorType = parser->previous.type;

    // Compile the operand
    parsePrecedence(parser, PREC_UNARY);

    // Emit the operator instrution
    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(parser, OP_NEGATE);
            break;
        case TOKEN_BANG:
            emitByte(parser, OP_NOT);
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
  { NULL,     NULL,    PREC_NONE },       // TOKEN_ERROR
  { NULL,     NULL,    PREC_NONE },       // TOKEN_EOF
};

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void parsePrecedence(Parser* parser, Precedence precedence) {
    advance(parser);
    ParseFn prefixRule = getRule(parser->previous.type)->prefix;
    if (prefixRule == NULL) {
        error(parser, "Expect expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(parser, canAssign);

    while (precedence <= getRule(parser->current.type)->precedence) {
        advance(parser);
        ParseFn infixRule = getRule(parser->previous.type)->infix;
        infixRule(parser, canAssign);
    }

    if (canAssign && match(parser, TOKEN_EQUAL)) {
        error(parser, "Invalid assigment target.");
    }
}

static void expression(Parser* parser) {
    parsePrecedence(parser, PREC_ASSIGNMENT);
}

static void varDeclaration(Parser* parser) {
    size_t global = parseVariable(parser, "Expect variable name.");

    if (match(parser, TOKEN_EQUAL)) {
        expression(parser);
    }
    else {
        emitByte(parser, OP_NIL);
    }
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(parser, global);
}

static void printStatement(Parser* parser) {
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after value for print.");
    emitByte(parser, OP_PRINT);
}

static void synchronize(Parser* parser) {
    parser->panicMode = false;

    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_SEMICOLON) {
            return;
        }

        switch (parser->current.type) {
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

        advance(parser);
    }
}

static void expressionStatement(Parser* parser) {
    expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(parser, OP_POP);
}

static void statement(Parser* parser) {
    if (match(parser, TOKEN_PRINT)) {
        printStatement(parser);
    }
    else {
        expressionStatement(parser);
    }
}

static void declaration(Parser* parser) {
    if (match(parser, TOKEN_VAR)) {
        varDeclaration(parser);
    }
    else {
        statement(parser);
    }

    if (parser->panicMode) {
        synchronize(parser);
    }
}

bool compile(VM* vm, const char* source, Chunk* chunk) {
    Scanner scanner;
    initScanner(&scanner, source);

    Parser parser;
    parser.scanner = &scanner;
    parser.currentChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;
    parser.previous.type = TOKEN_ERROR;
    parser.current.type = TOKEN_ERROR;
    parser.strings = vm->strings;
    parser.freeList = vm->freeList;

    advance(&parser);

    while(!match(&parser, TOKEN_EOF)) {
        declaration(&parser);
    }

    endCompiler(&parser);

    vm->freeList = parser.freeList;
    vm->strings = parser.strings;

    return !parser.hadError;
}