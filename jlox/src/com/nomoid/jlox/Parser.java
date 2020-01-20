package com.nomoid.jlox;

import java.util.List;

import static com.nomoid.jlox.TokenType.*;

class Parser {
    private static class ParseError extends RuntimeException {
        // SerialVersionUID to suppress warning
        private static final long serialVersionUID = 1L;
    }

    private final List<Token> tokens;
    private int current = 0;

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    Expr parse() {
        try {
            Expr expr = expression();
            if (!isAtEnd()) {
                throw error(peek(), "Unconsumed token.");
            }
            return expr;
        }
        catch (ParseError error) {
            return null;
        }
    }

    // expression     → comma ;
    private Expr expression() {
        return comma();
    }

    // comma          → ternary ( "," ternary )* ;
    private Expr comma() {
        if (match(COMMA)) {
            ternary();
            throw error(previous(), "Unary operator not supported.");
        }
        Expr expr = ternary();
        while (match(COMMA)) {
            Token operator = previous();
            Expr right = ternary();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    // ternary     → equality ("?" equality ":" equality)? ;
    private Expr ternary() {
        if (match(QUESTION, COLON)) {
            equality();
            throw error(previous(), "Unary operator not supported.");
        }
        Expr expr = equality();
        if (match(QUESTION)) {
            Token operator = previous();
            Expr center = equality();
            consume(COLON, "Expect ':' in ternary comparison.");
            Expr right = equality();
            expr = new Expr.Ternary(expr, operator, center, right);
        }
        return expr;
    }

    // equality       → comparison ( ( "!=" | "==" ) comparison )* ;
    private Expr equality() {
        if (match(BANG_EQUAL, EQUAL_EQUAL)) {
            comparison();
            throw error(previous(), "Unary operator not supported.");
        }
        Expr expr = comparison();
        while (match(BANG_EQUAL, EQUAL_EQUAL)) {
            Token operator = previous();
            Expr right = comparison();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    // comparison     → addition ( ( ">" | ">=" | "<" | "<=" ) addition )* ;
    private Expr comparison() {
        if (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
            addition();
            throw error(previous(), "Unary operator not supported.");
        }
        Expr expr = addition();
        while (match(GREATER, GREATER_EQUAL, LESS, LESS_EQUAL)) {
            Token operator = previous();
            Expr right = addition();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    // addition       → multiplication ( ( "-" | "+" ) multiplication )* ;
    private Expr addition() {
        // Note: Unary MINUS is supported
        if (match(PLUS)) {
            multiplication();
            throw error(previous(), "Unary operator not supported.");
        }
        Expr expr = multiplication();
        while (match(MINUS, PLUS)) {
            Token operator = previous();
            Expr right = multiplication();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    // multiplication → unary ( ( "/" | "*" ) unary )* ;
    private Expr multiplication() {
        if (match(SLASH, STAR)) {
            unary();
            throw error(previous(), "Unary operator not supported.");
        }
        Expr expr = unary();
        while (match(SLASH, STAR)) {
            Token operator = previous();
            Expr right = unary();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    // unary          → ( "!" | "-" ) unary
    //                | primary ;
    private Expr unary() {
        if (match(BANG, MINUS)) {
            Token operator = previous();
            Expr right = unary();
            return new Expr.Unary(operator, right);
        }

        return primary();
    }

    // primary        → NUMBER | STRING | "false" | "true" | "nil"
    //                | "(" expression ")" ;
    private Expr primary() {
        if (match(FALSE)) {
            return new Expr.Literal(false);
        }
        if (match(TRUE)) {
            return new Expr.Literal(true);
        }
        if (match(NIL)) {
            return new Expr.Literal(null);
        }

        if (match(NUMBER, STRING)) {
            return new Expr.Literal(previous().literal);
        }

        if (match(LEFT_PAREN)) {
            Expr expr = expression();
            consume(RIGHT_PAREN, "Expect ')' after expression.");
            return new Expr.Grouping(expr);
        }

        throw error(peek(), "Expect expression.");
    }

    private void synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous().type == SEMICOLON) {
                return;
            }

            switch (peek().type) {
                case CLASS:
                case FUN:
                case VAR:
                case FOR:
                case IF:
                case WHILE:
                case PRINT:
                case RETURN:
                    return;
                default:
                    break;
            }

            advance();
        }
    }

    private boolean match(TokenType... types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }

        return false;
    }

    private boolean check(TokenType type) {
        if (isAtEnd()) {
            return false;
        }
        return peek().type == type;
    }

    private Token advance() {
        if (!isAtEnd()) {
            current++;
        }
        return previous();
    }

    private boolean isAtEnd() {
        return peek().type == EOF;
    }

    private Token peek() {
        return tokens.get(current);
    }

    private Token previous() {
        return tokens.get(current - 1);
    }

    private Token consume(TokenType type, String message) {
        if (check(type)) {
            return advance();
        }
        throw error(peek(), message);
    }

    private ParseError error(Token token, String message) {
        Lox.error(token, message);
        return new ParseError();
    }
}