package com.nomoid.jlox;

import java.util.List;

interface Declaration {
    String name();
    List<Token> params();
    List<Stmt> body();
    Token token();
}

class FunctionDeclaration implements Declaration {

    private final Stmt.Function function;

    FunctionDeclaration(Stmt.Function function) {
        this.function = function;
    }

    @Override
    public String name() {
        return "<fn " + function.name.lexeme + ">";
    }

    @Override
    public List<Token> params() {
        return function.params;
    }

    @Override
    public List<Stmt> body() {
        return function.body;
    }

    @Override
    public Token token() {
        return function.name;
    }

}

class LambdaDeclaration implements Declaration {
    
    private final Expr.Lambda lambda;

    LambdaDeclaration(Expr.Lambda lambda) {
        this.lambda = lambda;
    }

    @Override
    public String name() {
        return "<lambda fn [at line " + lambda.keyword.line + "]>";
    }

    @Override
    public List<Token> params() {
        return lambda.params;
    }

    @Override
    public List<Stmt> body() {
        return lambda.body;
    }

    @Override
    public Token token() {
        return lambda.keyword;
    }
}