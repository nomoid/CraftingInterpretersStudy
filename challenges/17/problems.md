1.  Function trace:
    ```
    expression();
        parsePrecendence(PREC_ASSIGNMENT);
            grouping();
                expression();
                    parsePrecedence(PREC_ASSIGNMENT);
                        unary();
                            parsePrecedence(PREC_UNARY);
                                number();
                                    emitConstant(1);
                            emitByte(OP_NEGATE);
                        binary();
                            parsePrecedence(PREC_FACTOR);
                                number();
                                    emitConstant(2);
                            emitByte(OP_ADD);
            binary();
                parsePrecedence(PREC_UNARY);
                    number();
                        emitConstant(3);
                emitByte(OP_MULTIPLY);
            binary();
                parsePrecedence(PREC_FACTOR);
                    unary();
                        parsePrecedence(PREC_UNARY);
                            number();
                                emitConstant(4);
                        emitByte(OP_NEGATE);
                emitByte(OP_SUBTRACT);
    emitByte(OP_RETURN);
    ```
2.  In the full Lox langauge, the bang operator (`!`) can also be used as a prefix operator, but cannot be used in the infix position. In C specifically, the `*` and the `&` operators used for pointers are also both prefix and infix operators, but those do not appear in Lox.
3. Challenge problem skipped.