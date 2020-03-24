#include <stdio.h>
#include <stdlib.h>

#include "settings.h"
#include "common.h"
#include "compiler.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

static void expression(Parser* parser);
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

static void number(Parser* parser) {
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

static void string(Parser* parser) {
    emitConstant(parser, OBJ_VAL(copyString(
        &parser->freeList,
        &parser->strings,
        parser->previous.start + 1,
        parser->previous.length - 2)));
}

static void grouping(Parser* parser) {
    expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void binary(Parser* parser) {
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

static void literal(Parser* parser) {
    switch (parser->previous.type) {
        case TOKEN_FALSE: emitByte(parser, OP_FALSE); break;
        case TOKEN_TRUE: emitByte(parser, OP_TRUE); break;
        case TOKEN_NIL: emitByte(parser, OP_NIL); break;
        default:
            return; // Unreachable.
    }
}

static void unary(Parser* parser) {
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
  { NULL,     NULL,    PREC_NONE },       // TOKEN_IDENTIFIER
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

    prefixRule(parser);

    while (precedence <= getRule(parser->current.type)->precedence) {
        advance(parser);
        ParseFn infixRule = getRule(parser->previous.type)->infix;
        infixRule(parser);
    }
}

static void expression(Parser* parser) {
    parsePrecedence(parser, PREC_ASSIGNMENT);
}

bool compile(VM* vm, const char* source, Chunk* chunk) {
    Scanner scanner;
    initScanner(&scanner, source);

    Parser parser;
    initTable(&parser.strings);
    parser.scanner = &scanner;
    parser.currentChunk = chunk;
    parser.hadError = false;
    parser.panicMode = false;
    parser.previous.type = TOKEN_ERROR;
    parser.current.type = TOKEN_ERROR;

    advance(&parser);
    expression(&parser);
    consume(&parser, TOKEN_EOF, "Expect end of expression.");
    endCompiler(&parser);
    
    vm->freeList = parser.freeList;
    vm->strings = parser.strings;

    return !parser.hadError;
}