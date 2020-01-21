package com.nomoid.jlox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

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
import com.nomoid.jlox.Stmt.Block;
import com.nomoid.jlox.Stmt.Break;
import com.nomoid.jlox.Stmt.Expression;
import com.nomoid.jlox.Stmt.Function;
import com.nomoid.jlox.Stmt.If;
import com.nomoid.jlox.Stmt.Print;
import com.nomoid.jlox.Stmt.Return;
import com.nomoid.jlox.Stmt.Var;
import com.nomoid.jlox.Stmt.While;

class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
    private Interpreter interpreter;
    private final Stack<Map<String, Boolean>> scopes = new Stack<>();
    private FunctionType currentFunction = FunctionType.NONE;
    private BlockType currentBlock = BlockType.NONE;

    private enum FunctionType {
        NONE,
        FUNCTION
    }

    private enum BlockType {
        NONE,
        LOOP
    }

    Resolver(Interpreter interpreter) {
        this.interpreter = interpreter;
    }

    void resolve(List<Stmt> statements) {
        for (Stmt statement : statements) {
            resolve(statement);
        }
    }

    private void resolve(Stmt statement) {
        statement.accept(this);
    }

    private void resolve(Expr expr) {
        expr.accept(this);
    }

    private void beginScope() {
        scopes.push(new HashMap<String, Boolean>());
    }

    private void endScope() {
        scopes.pop();
    }

    private void declare(Token name) {
        if (scopes.isEmpty()) {
            return;
        }
        Map<String, Boolean> scope = scopes.peek();
        if (scope.containsKey(name.lexeme)) {
            Lox.error(name,
                "Variable with this name already declared in this scope.");
        }
        // Not yet ready for use
        scope.put(name.lexeme, false);
    }

    private void define(Token name) {
        if (scopes.isEmpty()) {
            return;
        }
        Map<String, Boolean> scope = scopes.peek();
        // Ready for use
        scope.put(name.lexeme, true);
    }

    private void resolveLocal(Expr expr, Token name) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            if (scopes.get(i).containsKey(name.lexeme)) {
                interpreter.resolve(expr, scopes.size() - 1 - i);
                return;
            }
        }
    }

    private void resolveFunction(Declaration declaration, FunctionType type) {
        FunctionType enclosingFunction = currentFunction;
        currentFunction = type;
        BlockType enclosingBlock = currentBlock;
        currentBlock = BlockType.NONE;
        beginScope();
        for (Token param : declaration.params()) {
            declare(param);
            define(param);
        }
        resolve(declaration.body());
        endScope();
        currentFunction = enclosingFunction;
        currentBlock = enclosingBlock;
    }

    @Override
    public Void visitBlockStmt(Block stmt) {
        beginScope();
        resolve(stmt.statements);
        endScope();
        return null;
    }

    @Override
    public Void visitBreakStmt(Break stmt) {
        if (currentBlock == BlockType.NONE) {
            Lox.error(stmt.token,
                "Cannot break from outside of for or while block.");
        }
        return null;
    }

    @Override
    public Void visitExpressionStmt(Expression stmt) {
        resolve(stmt.expression);
        return null;
    }

    @Override
    public Void visitFunctionStmt(Function stmt) {
        declare(stmt.name);
        define(stmt.name);
        resolveFunction(new FunctionDeclaration(stmt), FunctionType.FUNCTION);
        return null;
    }

    @Override
    public Void visitIfStmt(If stmt) {
        resolve(stmt.condition);
        resolve(stmt.thenBranch);
        if (stmt.elseBranch != null) {
            resolve(stmt.elseBranch);
        }
        return null;
    }

    @Override
    public Void visitPrintStmt(Print stmt) {
        resolve(stmt.expression);
        return null;
    }

    @Override
    public Void visitReturnStmt(Return stmt) {
        if (currentFunction == FunctionType.NONE) {
            Lox.error(stmt.keyword, "Cannot return from top-level code.");
        }

        if (stmt.value != null) {
            resolve(stmt.value);
        }
        return null;
    }

    @Override
    public Void visitVarStmt(Var stmt) {
        declare(stmt.name);
        if (stmt.initializer != null) {
            resolve(stmt.initializer);
        }
        define(stmt.name);
        return null;
    }

    @Override
    public Void visitWhileStmt(While stmt) {
        BlockType enclosingBlock = currentBlock;
        currentBlock = BlockType.LOOP;
        resolve(stmt.condition);
        resolve(stmt.body);
        currentBlock = enclosingBlock;
        return null;
    }

    @Override
    public Void visitAssignExpr(Assign expr) {
        resolve(expr.value);
        resolveLocal(expr, expr.name);
        return null;
    }

    @Override
    public Void visitBinaryExpr(Binary expr) {
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitCallExpr(Call expr) {
        resolve(expr.callee);
        for (Expr argument : expr.arguments) {
            resolve(argument);
        }
        return null;
    }

    @Override
    public Void visitGroupingExpr(Grouping expr) {
        resolve(expr.expression);
        return null;
    }

    @Override
    public Void visitLambdaExpr(Lambda expr) {
        resolveFunction(new LambdaDeclaration(expr), FunctionType.FUNCTION);
        return null;
    }

    @Override
    public Void visitLiteralExpr(Literal expr) {
        // Do nothing
        return null;
    }

    @Override
    public Void visitLogicalExpr(Logical expr) {
        resolve(expr.left);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitUnaryExpr(Unary expr) {
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitTernaryExpr(Ternary expr) {
        resolve(expr.left);
        resolve(expr.center);
        resolve(expr.right);
        return null;
    }

    @Override
    public Void visitVariableExpr(Variable expr) {
        if (!scopes.isEmpty() &&
                scopes.peek().get(expr.name.lexeme) == Boolean.FALSE) {
            Lox.error(expr.name,
                "Cannot read local variable in its own initializer.");
        }
        resolveLocal(expr, expr.name);
        return null;
    }
}