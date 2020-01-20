package com.nomoid.jlox;

import java.util.List;

import com.nomoid.jlox.Expr.Assign;
import com.nomoid.jlox.Expr.Binary;
import com.nomoid.jlox.Expr.Grouping;
import com.nomoid.jlox.Expr.Literal;
import com.nomoid.jlox.Expr.Ternary;
import com.nomoid.jlox.Expr.Unary;
import com.nomoid.jlox.Expr.Variable;
import com.nomoid.jlox.Stmt.Block;
import com.nomoid.jlox.Stmt.Expression;
import com.nomoid.jlox.Stmt.Print;
import com.nomoid.jlox.Stmt.Var;

class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {
    private Environment environment = new Environment();

    void interpret(List<Stmt> statements) {
        try {
            for (Stmt statement : statements) {
                execute(statement);
            }
        } catch (RuntimeError error) {
            Lox.runtimeError(error);
        }
    }

    private void execute(Stmt stmt) {
        stmt.accept(this);
    }

    void executeBlock(List<Stmt> statements, Environment environment) {
        Environment previous = this.environment;
        try {
            this.environment = environment;
            for (Stmt statement : statements) {
                execute(statement);
            }
        }
        finally {
            this.environment = previous;
        }
    }

    private Object evaluate(Expr expr) {
        return expr.accept(this);
    }

    @Override
    public Object visitBinaryExpr(Binary expr) {
        Object left = evaluate(expr.left);
        Object right = evaluate(expr.right);

        return binaryOp(left, expr.operator, right);
    }

    @Override
    public Object visitGroupingExpr(Grouping expr) {
        return evaluate(expr.expression);
    }

    @Override
    public Object visitLiteralExpr(Literal expr) {
        return expr.value;
    }

    @Override
    public Object visitUnaryExpr(Unary expr) {
        Object right = evaluate(expr.right);

        switch (expr.operator.type) {
        case BANG:
            return !isTruthy(right);
        case MINUS:
            checkNumberOperand(expr.operator, right);
            return -(double) right;
        default:
            // Unreachable code
            throw new RuntimeError(expr.operator, "Illegal unary operator.");
        }
    }

    @Override
    public Object visitTernaryExpr(Ternary expr) {
        Object left = evaluate(expr.left);

        switch (expr.operator.type) {
        // left ? center : right
        case QUESTION:
            if (isTruthy(left)) {
                return evaluate(expr.center);
            } else {
                return evaluate(expr.right);
            }
        default:
            // Unreachable code
            throw new RuntimeError(expr.operator, "Illegal ternary operator.");
        }
    }

    @Override
    public Object visitVariableExpr(Variable expr) {
        return environment.get(expr.name);
    }

    @Override
    public Object visitAssignExpr(Assign expr) {

        Token binaryOp = null;
        switch (expr.operator.type) {
        case PLUS_EQUAL:
            binaryOp = derivedToken(expr.operator, TokenType.PLUS);
            break;
        case MINUS_EQUAL:
            binaryOp = derivedToken(expr.operator, TokenType.MINUS);
            break;
        case STAR_EQUAL:
            binaryOp = derivedToken(expr.operator, TokenType.STAR);
            break;
        case SLASH_EQUAL:
            binaryOp = derivedToken(expr.operator, TokenType.SLASH);
            break;
        case EQUAL:
            // No binary op
            break;
        default:
            // Unreachable code
            throw new RuntimeError(expr.operator, "Illegal asign operator.");
        }

        Object value = evaluate(expr.value);
        if (binaryOp != null) {
            Object original = environment.get(expr.name);
            value = binaryOp(original, binaryOp, value);
        }
        environment.assign(expr.name, value);
        return value;
    }

    @Override
    public Void visitExpressionStmt(Expression stmt) {
        evaluate(stmt.expression);
        return null;
    }

    @Override
    public Void visitPrintStmt(Print stmt) {
        Object value = evaluate(stmt.expression);
        System.out.println(stringify(value));
        return null;
    }

    @Override
    public Void visitVarStmt(Var stmt) {
        Object value = null;
        if (stmt.initializer != null) {
            value = evaluate(stmt.initializer);
        }
        environment.define(stmt.name.lexeme, value);
        return null;
    }

    @Override
    public Void visitBlockStmt(Block stmt) {
        executeBlock(stmt.statements, new Environment(environment));
        return null;
    }

    private Object binaryOp(Object left, Token operator, Object right) {
        switch (operator.type) {
        case COMMA:
            // Ignore left expression result
            return right;
        case GREATER:
            checkNumberOperands(operator, left, right);
            return (double) left > (double) right;
        case GREATER_EQUAL:
            checkNumberOperands(operator, left, right);
            return (double) left >= (double) right;
        case LESS:
            checkNumberOperands(operator, left, right);
            return (double) left < (double) right;
        case LESS_EQUAL:
            checkNumberOperands(operator, left, right);
            return (double) left <= (double) right;
        case BANG_EQUAL:
            return !isEqual(left, right);
        case EQUAL_EQUAL:
            return isEqual(left, right);
        case MINUS:
            checkNumberOperands(operator, left, right);
            return (double) left - (double) right;
        case PLUS:
            if (left instanceof Double && right instanceof Double) {
                return (double) left + (double) right;
            }
            if (left instanceof String || right instanceof String) {
                return stringify(left) + stringify(right);
            }
            throw new RuntimeError(operator, "Operands must be two numbers or two strings.");
        case SLASH:
            checkNumberOperands(operator, left, right);
            if ((double) right == 0) {
                throw new RuntimeError(operator, "Division by zero.");
            }
            return (double) left / (double) right;
        case STAR:
            checkNumberOperands(operator, left, right);
            return (double) left * (double) right;
        default:
            // Unreachable code
            throw new RuntimeError(operator, "Illegal binary operator.");
        }
    }

    private boolean isTruthy(Object object) {
        if (object == null) {
            return false;
        }
        if (object instanceof Boolean) {
            return (boolean) object;
        }
        return true;
    }

    private boolean isEqual(Object a, Object b) {
        if (a == null && b == null) {
            return true;
        }
        if (a == null) {
            return false;
        }
        return a.equals(b);
    }

    private void checkNumberOperand(Token operator, Object operand) {
        if (operand instanceof Double) {
            return;
        }
        throw new RuntimeError(operator, "Operand must be a number.");
    }

    private void checkNumberOperands(Token operator, Object left, Object right) {
        if (left instanceof Double && right instanceof Double) {
            return;
        }
        throw new RuntimeError(operator, "Operand must be a number.");
    }

    private String stringify(Object object) {
        if (object == null) {
            return "nil";
        }

        if (object instanceof Double) {
            String text = object.toString();
            // Prevent the .0 from displaying for integers
            if (text.endsWith(".0")) {
                text = text.substring(0, text.length() - 2);
            }
            return text;
        }

        return object.toString();
    }

    private Token derivedToken(Token token, TokenType newType) {
        return new Token(newType, token.lexeme, null, token.line);
    }
}