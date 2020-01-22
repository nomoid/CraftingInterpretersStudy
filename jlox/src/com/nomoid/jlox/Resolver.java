package com.nomoid.jlox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

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
import com.nomoid.jlox.Stmt.Getter;
import com.nomoid.jlox.Stmt.If;
import com.nomoid.jlox.Stmt.Print;
import com.nomoid.jlox.Stmt.Return;
import com.nomoid.jlox.Stmt.Var;
import com.nomoid.jlox.Stmt.While;

class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
    private Interpreter interpreter;
    private final Stack<Map<String, ScopeDeclaration>> scopes = new Stack<>();
    private FunctionType currentFunction = FunctionType.NONE;
    private BlockType currentBlock = BlockType.NONE;
    private ClassType currentClass = ClassType.NONE;

    private static class ScopeDeclaration {
        ScopeDeclaration(Token token, DeclarationState state) {
            this.token = token;
            this.state = state;
        }

        ScopeDeclaration(ScopeDeclaration declaration, DeclarationState state) {
            this.token = declaration.token;
            this.state = state;
        }

        final DeclarationState state;
        final Token token;
    }

    private enum FunctionType {
        NONE, FUNCTION, INITIALIZER, METHOD
    }

    private enum BlockType {
        NONE, LOOP
    }

    private enum DeclarationState {
        DECLARED, INITIALIZED, USED
    }

    private enum ResolveLocalType {
        USE, ASSIGN, THIS;
    }

    private enum ClassType {
        NONE, CLASS
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
        scopes.push(new HashMap<String, ScopeDeclaration>());
    }

    private void endScope() {
        Map<String, ScopeDeclaration> scope = scopes.peek();
        for (Map.Entry<String, ScopeDeclaration> state : scope.entrySet()) {
            if (state.getValue().state != DeclarationState.USED) {
                if (Lox.strict) {
                    Lox.error(state.getValue().token, "Variable with this name is never used.");
                }
            }
        }
        scopes.pop();
    }

    private void declare(Token name) {
        if (scopes.isEmpty()) {
            return;
        }
        Map<String, ScopeDeclaration> scope = scopes.peek();
        if (scope.containsKey(name.lexeme)) {
            Lox.error(name, "Variable with this name already declared in this scope.");
        }
        // Not yet ready for use
        scope.put(name.lexeme, new ScopeDeclaration(name, DeclarationState.DECLARED));
    }

    private void define(Token name) {
        if (scopes.isEmpty()) {
            return;
        }
        Map<String, ScopeDeclaration> scope = scopes.peek();
        ScopeDeclaration oldDeclaration = scope.get(name.lexeme);
        // Ready for use
        scope.put(name.lexeme, new ScopeDeclaration(oldDeclaration, DeclarationState.INITIALIZED));
    }

    private void resolveLocal(Expr expr, Token name, ResolveLocalType type) {
        for (int i = scopes.size() - 1; i >= 0; i--) {
            Map<String, ScopeDeclaration> scope = scopes.get(i);
            if (scope.containsKey(name.lexeme)) {
                ScopeDeclaration oldDeclaration;
                switch (type) {
                case USE:
                    // Using a variable
                    oldDeclaration = scope.get(name.lexeme);
                    scope.put(name.lexeme, new ScopeDeclaration(oldDeclaration, DeclarationState.USED));
                    break;
                case ASSIGN:
                    oldDeclaration = scope.get(name.lexeme);
                    if (oldDeclaration.state == DeclarationState.DECLARED) {
                        // First time defining
                        scope.put(name.lexeme, new ScopeDeclaration(oldDeclaration, DeclarationState.INITIALIZED));
                    }
                    break;
                case THIS:
                    // Do nothing;
                    break;
                }
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
            Lox.error(stmt.token, "Cannot break from outside of for or while block.");
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
            if (currentFunction == FunctionType.INITIALIZER) {
                Lox.error(stmt.keyword, "Cannot return a value from an initializer.");
            }
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
        if (Lox.strict) {
            if (stmt.initializer != null) {
                define(stmt.name);
            }
        } else {
            define(stmt.name);
        }
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
        resolveLocal(expr, expr.name, ResolveLocalType.ASSIGN);
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
        if (!scopes.isEmpty()) {
            ScopeDeclaration decl = scopes.peek().get(expr.name.lexeme);
            if (decl != null && decl.state == DeclarationState.DECLARED) {
                if (Lox.strict) {
                    Lox.error(expr.name, "Variable with this name is guaranteed to be " + " uninitialzed.");
                } else {
                    Lox.error(expr.name, "Cannot access variable in its own initializer.");
                }
            }
        }
        resolveLocal(expr, expr.name, ResolveLocalType.USE);
        return null;
    }

    @Override
    public Void visitClassStmt(Class stmt) {
        ClassType enclosingClass = currentClass;
        currentClass = ClassType.CLASS;

        declare(stmt.name);
        define(stmt.name);

        beginScope();
        // Don't need to track usage for 'this'
        scopes.peek().put("this", new ScopeDeclaration(stmt.name, DeclarationState.USED));
        for (Stmt.Function method : stmt.methods) {
            FunctionType declaration = FunctionType.METHOD;
            if (method.name.lexeme.equals(Interpreter.INIT_STRING)) {
                declaration = FunctionType.INITIALIZER;
            }
            resolveFunction(new FunctionDeclaration(method), declaration);
        }
        // 'this' for class is the class object itself
        for (Stmt.Function staticFunction : stmt.statics) {
            resolveFunction(new FunctionDeclaration(staticFunction), FunctionType.METHOD);
        }
        // Resolve getters
        for (Stmt.Getter getter : stmt.getters) {
            resolveFunction(new GetterDeclaration(getter), FunctionType.METHOD);
        }
        endScope();

        currentClass = enclosingClass;

        return null;
    }

    @Override
    public Void visitGetExpr(Get expr) {
        resolve(expr.object);
        return null;
    }

    @Override
    public Void visitSetExpr(Set expr) {
        resolve(expr.value);
        resolve(expr.object);
        return null;
    }

    @Override
    public Void visitThisExpr(This expr) {
        if (currentClass == ClassType.NONE) {
            Lox.error(expr.keyword, "Cannot use 'this' outside of a class.");
            return null;
        }
        resolveLocal(expr, expr.keyword, ResolveLocalType.THIS);
        return null;
    }

    @Override
    public Void visitGetterStmt(Getter stmt) {
        throw new InterpreterException(stmt.name,
            "Getters cannot be visited outside of class context.");
    }
}