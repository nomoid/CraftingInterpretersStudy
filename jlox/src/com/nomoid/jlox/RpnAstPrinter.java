package com.nomoid.jlox;

import com.nomoid.jlox.Expr.Assign;
import com.nomoid.jlox.Expr.Binary;
import com.nomoid.jlox.Expr.Grouping;
import com.nomoid.jlox.Expr.Literal;
import com.nomoid.jlox.Expr.Logical;
import com.nomoid.jlox.Expr.Ternary;
import com.nomoid.jlox.Expr.Unary;
import com.nomoid.jlox.Expr.Variable;
import com.nomoid.jlox.Expr.Visitor;

class RpnAstPrinter implements Visitor<String> {

    String print(Expr expr) {
        return expr.accept(this);
    }

    @Override
    public String visitBinaryExpr(Binary expr) {
        return rpn(expr.operator.lexeme, expr.left, expr.right);
    }

    @Override
    public String visitGroupingExpr(Grouping expr) {
        return rpn("group", expr.expression);
    }

    @Override
    public String visitLiteralExpr(Literal expr) {
        if (expr.value == null) {
            return "nil";
        }
        return expr.value.toString();
    }

    @Override
    public String visitUnaryExpr(Unary expr) {
        return rpn(expr.operator.lexeme, expr.right);
    }

    @Override
    public String visitTernaryExpr(Ternary expr) {
        return rpn(expr.operator.lexeme, expr.left, expr.center, expr.right);
    }

    @Override
    public String visitVariableExpr(Variable expr) {
        return "$" + expr.name.lexeme;
    }

    @Override
    public String visitAssignExpr(Assign expr) {
        return rpn(expr.operator.lexeme, new Variable(expr.name), expr.value);
    }
    
    @Override
    public String visitLogicalExpr(Logical expr) {
        return rpn(expr.operator.lexeme, expr.left, expr.right);
    }

    private String rpn(String name, Expr... exprs) {
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