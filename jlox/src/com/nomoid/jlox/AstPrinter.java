package com.nomoid.jlox;

import com.nomoid.jlox.Expr.Assign;
import com.nomoid.jlox.Expr.Binary;
import com.nomoid.jlox.Expr.Call;
import com.nomoid.jlox.Expr.Grouping;
import com.nomoid.jlox.Expr.Lambda;
import com.nomoid.jlox.Expr.Literal;
import com.nomoid.jlox.Expr.Logical;
import com.nomoid.jlox.Expr.Ternary;
import com.nomoid.jlox.Expr.Unary;
import com.nomoid.jlox.Expr.Variable;

// Creates an unambiguous Lisp-like string representation of AST nodes
class AstPrinter implements Expr.Visitor<String> {
    String print(Expr expr) {
        return expr.accept(this);
    }

    @Override
    public String visitBinaryExpr(Binary expr) {
        return parenthesize(expr.operator.lexeme, expr.left, expr.right);
    }

    @Override
    public String visitGroupingExpr(Grouping expr) {
        return parenthesize("group", expr.expression);
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
        return parenthesize(expr.operator.lexeme, expr.right);
    }

    @Override
    public String visitTernaryExpr(Ternary expr) {
        return parenthesize(expr.operator.lexeme, expr.left, expr.center, expr.right);
    }

    @Override
    public String visitVariableExpr(Variable expr) {
        return "$" + expr.name.lexeme;
    }

    @Override
    public String visitAssignExpr(Assign expr) {
        return parenthesize(expr.operator.lexeme, new Variable(expr.name), expr.value);
    }

    @Override
    public String visitLogicalExpr(Logical expr) {
        return parenthesize(expr.operator.lexeme, expr.left, expr.right);
    }

    @Override
    public String visitCallExpr(Call expr) {
        return parenthesize(expr.callee.accept(this), expr.arguments.toArray(new Expr[] {}));
    }

    @Override
    public String visitLambdaExpr(Lambda expr) {
        throw new UnsupportedOperationException("Lambda expressions are not currently supported.");
    }

    private String parenthesize(String name, Expr... exprs) {
        StringBuilder builder = new StringBuilder();

        builder.append("(").append(name);
        for (Expr expr : exprs) {
            builder.append(" ");
            builder.append(expr.accept(this));
        }
        builder.append(")");

        return builder.toString();
    }

    public static void main(String[] args) {
        Expr expression = new Expr.Binary(
                new Expr.Unary(new Token(TokenType.MINUS, "-", null, 1), new Expr.Literal(123)),
                new Token(TokenType.STAR, "*", null, 1), new Expr.Grouping(new Expr.Literal(45.67)));

        System.out.println(new AstPrinter().print(expression));
    }
}