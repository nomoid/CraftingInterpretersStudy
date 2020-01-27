#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

Scanner scanner;

void compile(const char* source) {
    initScanner(&scanner, source);
    size_t line = (size_t)-1;
    while (1) {
        Token token;
        scanToken(&scanner, &token);
        if (token.line != line) {
            printf("%4ld ", token.line);
            line = token.line;
        }
        else {
            printf("   | ");
        }
        printf("%2d '%.*s'\n", token.type, token.length, token.start);

        if (token.type == TOKEN_EOF) {
            break;
        }
    }
}