package com.nomoid.jlox;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static com.nomoid.jlox.TokenType.*;

class Parser {
    public static final int MAX_PARAM_SIZE = 255;
    private static class ParseError extends RuntimeException {
        // SerialVersionUID to suppress warning
        private static final long serialVersionUID = 1L;
    }

    private final List<Token> tokens;
    private int current = 0;

    Parser(List<Token> tokens) {
        this.tokens = tokens;
    }

    // program     → declaration* EOF ;
    List<Stmt> parse() {
        try {
            List<Stmt> statements = new ArrayList<>();
            while (!isAtEnd()) {
                statements.add(declaration());
            }
            return statements;
        }
        catch (ParseError error) {
            return null;
        }
    }

    Expr parseExpression() {
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

    // declaration → classDecl
    //             | funDecl
    //             | varDecl
    //             | statement ;
    private Stmt declaration() {
        try {
            if (match(CLASS)) {
                return classDeclaration();
            }
            if (peek().type == FUN) {
                // Lookahead 2 for lambdas
                if (peekNext().type == IDENTIFIER) {
                    consume(FUN, "Expected 'fun' from peek.");
                    return function("function");
                }
            }
            if (match(VAR)) {
                return varDeclaration();
            }
            return statement();
        }
        catch (ParseError error) {
            synchronize();
            return null;
        }
    }

    // classDecl   → "class" IDENTIFIER "{" function* "}" ;
    private Stmt classDeclaration() {
        Token name = consume(IDENTIFIER, "Expect class name.");
        consume(LEFT_BRACE, "Expect '{' before class body.");
        List<Stmt.Function> methods = new ArrayList<>();
        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            methods.add(function("method"));
        }
        consume(RIGHT_BRACE, "Expect '}' after class body.");
        return new Stmt.Class(name, methods);
    }

    // funDecl  → "fun" function ;
    // function → IDENTIFIER "(" parameters? ")" block ;
    // parameters → IDENTIFIER ( "," IDENTIFIER )* ;
    private Stmt.Function function(String kind) {
        Token name = consume(IDENTIFIER, "Expect " + kind + " name.");
        consume(LEFT_PAREN, "Expect '(' after " + kind + " name.");
        List<Token> parameters = new ArrayList<>();
        if (!check(RIGHT_PAREN)) {
            do {
                if (parameters.size() >= MAX_PARAM_SIZE) {
                    error(peek(), "Cannot have more than " +
                        MAX_PARAM_SIZE + " parameters.");
                }
                parameters.add(consume(IDENTIFIER, "Expect parameter name."));
            }
            while (match(COMMA));
        }
        consume(RIGHT_PAREN, "Expect ')' after parameters.");

        consume(LEFT_BRACE, "Expect '{' before " + kind + " body.");
        List<Stmt> body = block();
        return new Stmt.Function(name, parameters, body);
    }

    // varDecl → "var" IDENTIFIER ( "=" expression )? ";" ;
    private Stmt varDeclaration() {
        Token name = consume(IDENTIFIER, "Expect variable name.");

        Expr initializer = null;
        if (match(EQUAL)) {
            initializer = expression();
        }

        consume(SEMICOLON, "Expect ';' after variable declaration.");
        return new Stmt.Var(name, initializer);
    }

    // statement   → breakStmt
    //             | exprStmt
    //             | forStmt
    //             | ifStmt
    //             | printStmt
    //             | returnStmt
    //             | whileStmt
    //             | block ;
    private Stmt statement() {
        if (match(BREAK)) {
            return breakStatement();
        }
        if (match(FOR)) {
            return forStatement();
        }
        if (match(IF)) {
            return ifStatement();
        }
        if (match(PRINT)) {
            return printStatement();
        }
        if (match(RETURN)) {
            return returnStatement();
        }
        if (match(WHILE)) {
            return whileStatement();
        }
        if (match(LEFT_BRACE)) {
            return new Stmt.Block(block());
        }
        return expressionStatement();
    }

    // returnStmt → "return" expression? ";" ;
    private Stmt returnStatement() {
        Token keyword = previous();
        Expr value = null;
        if (!check(SEMICOLON)) {
            value = expression();
        }
        consume(SEMICOLON, "Expect ';' after return value.");
        return new Stmt.Return(keyword, value);
    }

    // breakStmt → "break" ";" ;
    private Stmt breakStatement() {
        Token token = previous();
        consume(SEMICOLON, "Expect ';' after value.");
        return new Stmt.Break(token);
    }

    // forStmt   → "for" "(" ( varDecl | exprStmt | ";" )
    //                       expression? ";"
    //                       expression? ")" statement ;
    private Stmt forStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'for'.");
        Stmt initializer;
        if (match(SEMICOLON)) {
            initializer = null;
        }
        else if (match(VAR)) {
            initializer = varDeclaration();
        }
        else {
            initializer = expressionStatement();
        }

        Expr condition = null;
        if (!check(SEMICOLON)) {
            condition = expression();
        }
        consume(SEMICOLON, "Expect ';' after loop condition.");

        Expr increment = null;
        if (!check(RIGHT_PAREN)) {
            increment = expression();
        }
        consume(RIGHT_PAREN, "Expect ')' after for clauses");

        Stmt body = statement();

        if (increment != null) {
            body = new Stmt.Block(Arrays.asList(
                body,
                new Stmt.Expression(increment)
            ));
        }

        if (condition == null) {
            condition = new Expr.Literal(true);
        }
        body = new Stmt.While(condition, body);

        if (initializer != null) {
            body = new Stmt.Block(Arrays.asList(initializer, body));
        }
        
        return body;
    }

    // ifStmt    → "if" "(" expression ")" statement ( "else" statement )? ;
    private Stmt ifStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'if'.");
        Expr condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after if condition.");

        Stmt thenBranch = statement();
        Stmt elseBranch = null;
        if (match(ELSE)) {
            elseBranch = statement();
        }

        return new Stmt.If(condition, thenBranch, elseBranch);
    }

    // whileStmt → "while" "(" expression ")" statement ;
    private Stmt whileStatement() {
        consume(LEFT_PAREN, "Expect '(' after 'while'.");
        Expr condition = expression();
        consume(RIGHT_PAREN, "Expect ')' after while condition.");

        Stmt body = statement();

        return new Stmt.While(condition, body);
    }

    // block     → "{" declaration* "}" ;
    private List<Stmt> block() {
        List<Stmt> statements = new ArrayList<>();

        while (!check(RIGHT_BRACE) && !isAtEnd()) {
            statements.add(declaration());
        }

        consume(RIGHT_BRACE, "Expect '}' after block.");
        return statements;
    }

    // printStmt → "print" expression ";" ;
    private Stmt printStatement() {
        Expr value = expression();
        consume(SEMICOLON, "Expect ';' after value.");
        return new Stmt.Print(value);
    }

    // exprStmt  → expression ";" ;
    private Stmt expressionStatement() {
        Expr value = expression();
        consume(SEMICOLON, "Expect ';' after expression.");
        return new Stmt.Expression(value);
    }

    // expression     → comma ;
    private Expr expression() {
        return comma();
    }

    // comma          → assignment ( "," assignment )* ;
    private Expr comma() {
        if (match(COMMA)) {
            assignment();
            throw error(previous(), "Unary operator not supported.");
        }
        Expr expr = assignment();
        while (match(COMMA)) {
            Token operator = previous();
            Expr right = assignment();
            expr = new Expr.Binary(expr, operator, right);
        }
        return expr;
    }

    // assignment → ( call "." )? IDENTIFIER ( "+" | "-" | "*" | "/" )? "=" assignment
    //            | ternary ;
    private Expr assignment() {
        if (match(EQUAL, PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL)) {
            ternary();
            throw error(previous(), "Unary operator not supported.");
        }
        Expr expr = ternary();
        if (match(EQUAL, PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL)) {
            Token equals = previous();
            Expr value = assignment();
            if (expr instanceof Expr.Variable) {
                Token name = ((Expr.Variable)expr).name;
                return new Expr.Assign(name, equals, value);
            }
            else if (expr instanceof Expr.Get) {
                Expr.Get get = (Expr.Get)expr;
                return new Expr.Set(get.object, get.name, equals, value);
            }
            error(equals, "Invalid assignment target.");
        }

        return expr;
    }

    // ternary     → logic_or ("?" logic_or ":" logic_or)? ;
    private Expr ternary() {
        if (match(QUESTION, COLON)) {
            or();
            throw error(previous(), "Unary operator not supported.");
        }
        Expr expr = or();
        if (match(QUESTION)) {
            Token operator = previous();
            Expr center = or();
            consume(COLON, "Expect ':' in ternary comparison.");
            Expr right = or();
            expr = new Expr.Ternary(expr, operator, center, right);
        }
        return expr;
    }

    // logic_or   → logic_and ( "or" logic_and )* ;
    private Expr or() {
        if (match(OR)) {
            and();
            throw error(previous(), "Unary operator not supported.");
        }
        Expr expr = and();
        if (match(OR)) {
            Token operator = previous();
            Expr right = and();
            expr = new Expr.Logical(expr, operator, right);
        }

        return expr;
    }

    // logic_and  → equality ( "and" equality )* ;
    private Expr and() {
        if (match(AND)) {
            equality();
            throw error(previous(), "Unary operator not supported.");
        }
        Expr expr = equality();
        if (match(AND)) {
            Token operator = previous();
            Expr right = equality();
            expr = new Expr.Logical(expr, operator, right);
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

    // unary          → ( "!" | "-" ) unary | call ;
    private Expr unary() {
        if (match(BANG, MINUS)) {
            Token operator = previous();
            Expr right = unary();
            return new Expr.Unary(operator, right);
        }

        return call();
    }

    // call           → lambda ( "(" arguments? ")" | "." IDENTIFIER )* ;
    private Expr call() {
        Expr expr = lambda();
        while (true) {
            if (match(LEFT_PAREN)) {
                expr = finishCall(expr);
            }
            else if (match(DOT)) {
                Token name = consume(IDENTIFIER,
                    "Expect property name after '.'.");
                expr = new Expr.Get(expr, name);
            }
            else {
                break;
            }
        }
        return expr;
    }

    private Expr finishCall(Expr callee) {
        List<Expr> arguments = new ArrayList<>();
        if (!check(RIGHT_PAREN)) {
            do {
                if (arguments.size() >= MAX_PARAM_SIZE) {
                    error(peek(), "Cannot have more than " + MAX_PARAM_SIZE + " arguments.");
                }
                // Cannot match expression() because of 'comma' expression
                arguments.add(assignment());
            }
            while (match(COMMA));
        }

        Token paren = consume(RIGHT_PAREN, "Expect ')' after arguments.");

        return new Expr.Call(callee, paren, arguments);
    }

    // lambda       → "fun" lambdaInside | primary ;
    // lambdaInside → "(" parameters? ")" block ;
    // parameters   → IDENTIFIER ( "," IDENTIFIER )* ;
    private Expr lambda() {
        if (match(FUN)) {
            return lambdaInside();
        }
        return primary();
    }

    private Expr lambdaInside() {
        if (peek().type == IDENTIFIER) {
            consume(LEFT_PAREN,
                "Lambda declaration cannot have function name.");
        }
        consume(LEFT_PAREN, "Expect '(' for lambda declaration.");
        List<Token> parameters = new ArrayList<>();
        if (!check(RIGHT_PAREN)) {
            do {
                if (parameters.size() >= MAX_PARAM_SIZE) {
                    error(peek(), "Cannot have more than " +
                        MAX_PARAM_SIZE + " parameters.");
                }
                parameters.add(consume(IDENTIFIER, "Expect parameter name."));
            }
            while (match(COMMA));
        }
        Token paren = consume(RIGHT_PAREN, "Expect ')' after parameters.");

        consume(LEFT_BRACE, "Expect '{' before lambda body.");
        List<Stmt> body = block();
        return new Expr.Lambda(paren, parameters, body);
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

        if (match(THIS)) {
            return new Expr.This(previous());
        }

        if (match(IDENTIFIER)) {
            return new Expr.Variable(previous());
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

    // 2x lookahead, only for lambdas
    private Token peekNext() {
        if (current + 1 == tokens.size()) {
            return tokens.get(current);
        }
        return tokens.get(current + 1);
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