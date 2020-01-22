package com.nomoid.jlox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.nomoid.jlox.Expr.Assign;
import com.nomoid.jlox.Expr.Binary;
import com.nomoid.jlox.Expr.Call;
import com.nomoid.jlox.Expr.Get;
import com.nomoid.jlox.Expr.Grouping;
import com.nomoid.jlox.Expr.Lambda;
import com.nomoid.jlox.Expr.Literal;
import com.nomoid.jlox.Expr.Logical;
import com.nomoid.jlox.Expr.Set;
import com.nomoid.jlox.Expr.Ternary;
import com.nomoid.jlox.Expr.This;
import com.nomoid.jlox.Expr.Unary;
import com.nomoid.jlox.Expr.Variable;
import com.nomoid.jlox.Stmt.Block;
import com.nomoid.jlox.Stmt.Break;
import com.nomoid.jlox.Stmt.Class;
import com.nomoid.jlox.Stmt.Expression;
import com.nomoid.jlox.Stmt.Function;
import com.nomoid.jlox.Stmt.If;
import com.nomoid.jlox.Stmt.Print;
import com.nomoid.jlox.Stmt.Return;
import com.nomoid.jlox.Stmt.Var;
import com.nomoid.jlox.Stmt.While;

class Interpreter implements Expr.Visitor<Object>, Stmt.Visitor<Void> {
    final Environment globals = new Environment();
    private Environment environment = globals;
    private final Map<Expr, Integer> locals = new HashMap<>();
    static final String INIT_STRING = "init";

    Interpreter() {
        globals.define("clock", new LoxCallable() {

            @Override
            public Object call(Interpreter interpreter, List<Object> arguments) {
                return (double) System.currentTimeMillis() / 1000;
            }

            @Override
            public int arity() {
                return 0;
            }

            @Override
            public String toString() {
                return "<native fn clock>";
            }
        });
    }

    void interpret(List<Stmt> statements) {
        try {
            try {
                for (Stmt statement : statements) {
                    execute(statement);
                }
            } catch (BreakError error) {
                throw new RuntimeError(error.token, "Unexpected break outside of for or while block.");
            } catch (ReturnError error) {
                throw new RuntimeError(error.token, "Unexpected return outside of function declaration.");
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
        } finally {
            this.environment = previous;
        }
    }

    private Object evaluate(Expr expr) {
        return expr.accept(this);
    }

    void resolve(Expr expr, int depth) {
        locals.put(expr, depth);
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
            throw new InterpreterException(expr.operator, "Illegal unary operator.");
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
            throw new InterpreterException(expr.operator, "Illegal ternary operator.");
        }
    }

    @Override
    public Object visitVariableExpr(Variable expr) {
        return lookUpVariable(expr.name, expr);
    }

    @Override
    public Object visitAssignExpr(Assign expr) {
        Token binaryOp = getAssignBinaryOp(expr.operator);

        Object value = evaluate(expr.value);
        Integer distance = locals.get(expr);
        if (binaryOp != null) {
            Object original;
            if (distance != null) {
                original = environment.getAt(distance, expr.name);
            } else {
                original = globals.get(expr.name);
            }
            value = binaryOp(original, binaryOp, value);
        }
        if (distance != null) {
            environment.assignAt(distance, expr.name, value);
        } else {
            globals.assign(expr.name, value);
        }
        return value;
    }

    @Override
    public Object visitLogicalExpr(Logical expr) {
        Object left = evaluate(expr.left);
        if (expr.operator.type == TokenType.OR) {
            if (isTruthy(left)) {
                return left;
            }
        } else {
            if (!isTruthy(left)) {
                return left;
            }
        }
        return evaluate(expr.right);
    }

    @Override
    public Object visitCallExpr(Call expr) {
        Object callee = evaluate(expr.callee);

        List<Object> arguments = new ArrayList<>();
        for (Expr argument : expr.arguments) {
            arguments.add(evaluate(argument));
        }

        if (!(callee instanceof LoxCallable)) {
            throw new RuntimeError(expr.paren, "Can only call functions and classes");
        }

        LoxCallable function = (LoxCallable) callee;
        if (arguments.size() != function.arity()) {
            throw new RuntimeError(expr.paren,
                    "Expected " + function.arity() + " arguments but got " + arguments.size() + ".");
        }
        return function.call(this, arguments);
    }

    @Override
    public Object visitLambdaExpr(Lambda expr) {
        return new LoxFunction(expr, environment, false);
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
        } else {
            value = Environment.UNINITIALIZED;
        }
        environment.define(stmt.name.lexeme, value);
        return null;
    }

    @Override
    public Void visitBlockStmt(Block stmt) {
        executeBlock(stmt.statements, new Environment(environment));
        return null;
    }

    @Override
    public Void visitIfStmt(If stmt) {
        if (isTruthy(evaluate(stmt.condition))) {
            execute(stmt.thenBranch);
        } else if (stmt.elseBranch != null) {
            execute(stmt.elseBranch);
        }
        return null;
    }

    @Override
    public Void visitWhileStmt(While stmt) {
        try {
            while (isTruthy(evaluate(stmt.condition))) {
                execute(stmt.body);
            }
        } catch (BreakError error) {
            // Break out of for/while loop
        }
        return null;
    }

    @Override
    public Void visitBreakStmt(Break stmt) {
        throw new BreakError(stmt.token);
    }

    @Override
    public Void visitFunctionStmt(Function stmt) {
        LoxFunction function = new LoxFunction(stmt, environment, false);
        environment.define(stmt.name.lexeme, function);
        return null;
    }

    @Override
    public Void visitReturnStmt(Return stmt) {
        Object value = null;
        if (stmt.value != null) {
            value = evaluate(stmt.value);
        }
        throw new ReturnError(stmt.keyword, value);
    }

    @Override
    public Void visitClassStmt(Class stmt) {
        environment.define(stmt.name.lexeme, null);

        Map<String, LoxFunction> statics = new HashMap<>();
        for (Stmt.Function staticFunction : stmt.statics) {
            LoxFunction function = new LoxFunction(staticFunction, environment,
                false);
            statics.put(staticFunction.name.lexeme, function);
        }
        Map<String, LoxFunction> methods = new HashMap<>();
        for (Stmt.Function method : stmt.methods) {
            LoxFunction function = new LoxFunction(method, environment,
                method.name.lexeme.equals(INIT_STRING));
            methods.put(method.name.lexeme, function);
        }

        LoxClass klass = new LoxClass(stmt.name.lexeme, methods, statics);
        environment.assign(stmt.name, klass);
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
            throw new InterpreterException(operator, "Illegal binary operator.");
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

    private Object lookUpVariable(Token name, Expr expr) {
        Integer distance = locals.get(expr);
        if (distance != null) {
            return environment.getAt(distance, name);
        } else {
            return globals.get(name);
        }
    }

    @Override
    public Object visitGetExpr(Get expr) {
        Object object = evaluate(expr.object);
        if (object instanceof LoxInstance) {
            return ((LoxInstance) object).get(expr.name);
        }

        throw new RuntimeError(expr.name, "Only instances have properties.");
    }

    @Override
    public Object visitSetExpr(Set expr) {
        Object object = evaluate(expr.object);
        if (!(object instanceof LoxInstance)) {
            throw new RuntimeError(expr.name, "Only instances have fields.");
        }
        LoxInstance instance = (LoxInstance) object;

        Token binaryOp = getAssignBinaryOp(expr.operator);

        Object value = evaluate(expr.value);
        if (binaryOp != null) {
            Object original = instance.get(expr.name);
            value = binaryOp(original, binaryOp, value);
        }
        instance.set(expr.name, value);
        return value;
    }

    private Token getAssignBinaryOp(Token operator) {
        switch (operator.type) {
        case PLUS_EQUAL:
            return derivedToken(operator, TokenType.PLUS);
        case MINUS_EQUAL:
            return derivedToken(operator, TokenType.MINUS);
        case STAR_EQUAL:
            return derivedToken(operator, TokenType.STAR);
        case SLASH_EQUAL:
            return derivedToken(operator, TokenType.SLASH);
        case EQUAL:
            // No binary op
            return null;
        default:
            // Unreachable code
            throw new InterpreterException(operator, "Illegal asign operator.");
        }
    }

    @Override
    public Object visitThisExpr(This expr) {
        return lookUpVariable(expr.keyword, expr);
    }
}