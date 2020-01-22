package com.nomoid.jlox;

import com.nomoid.jlox.Expr.Visitor;

class RpnAstPrinter extends AstPrinter implements Visitor<String> {

    @Override
    protected String parenthesize(String name, Expr... exprs) {
        StringBuilder builder = new StringBuilder();

        for (Expr expr : exprs) {
            builder.append(expr.accept(this));
            builder.append(" ");
        }
        builder.append(name);

        return builder.toString();
    }

    public static void main(String[] args) {
        // Expr expression = new Expr.Binary(
        // new Expr.Unary(
        // new Token(TokenType.MINUS, "-", null, 1),
        // new Expr.Literal(123)),
        // new Token(TokenType.STAR, "*", null, 1),
        // new Expr.Grouping(
        // new Expr.Literal(45.67)));

        Expr expression = new Expr.Binary(
                new Expr.Binary(new Expr.Literal(1), new Token(TokenType.PLUS, "+", null, 1), new Expr.Literal(2)),
                new Token(TokenType.STAR, "*", null, 1),
                new Expr.Binary(new Expr.Literal(4), new Token(TokenType.MINUS, "-", null, 1), new Expr.Literal(3)));

        System.out.println(new RpnAstPrinter().print(expression));
    }
}