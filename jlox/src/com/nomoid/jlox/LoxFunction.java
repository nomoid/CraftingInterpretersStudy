package com.nomoid.jlox;

import java.util.List;

class LoxFunction implements LoxCallable {

    private final Declaration declaration;
    private final Environment closure;
    private final boolean isInitializer;

    LoxFunction(Stmt.Function declaration, Environment closure,
            boolean isInitializer) {
        this(new FunctionDeclaration(declaration), closure, isInitializer);
    }

    LoxFunction(Expr.Lambda declaration, Environment closure,
            boolean isInitializer) {
        this(new LambdaDeclaration(declaration), closure, isInitializer);
    }

    LoxFunction(Declaration declaration, Environment closure,
            boolean isInitializer) {
        this.closure = closure;
        this.declaration = declaration;
        this.isInitializer = isInitializer;
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        Environment environment = new Environment(closure);
        for (int i = 0; i < declaration.params().size(); i++) {
            environment.define(declaration.params().get(i).lexeme,
                arguments.get(i));
        }
        try {
            interpreter.executeBlock(declaration.body(), environment);
        }
        catch (BreakError error) {
            throw new RuntimeError(error.token, "Unexpected break outside of for or while block.");
        }
        catch (ReturnError returnValue) {
            if (isInitializer) {
                return closure.getAt(0, new Token(TokenType.THIS, "this",
                    null, declaration.token().line));
            }
            return returnValue.value;
        }
        if (isInitializer) {
            return closure.getAt(0, new Token(TokenType.THIS, "this",
                null, declaration.token().line));
        }
        return null;
    }

    @Override
    public int arity() {
        return declaration.params().size();
    }

    @Override
    public String toString() {
        return declaration.name();
    }

    LoxFunction bind(LoxInstance instance) {
        Environment environment = new Environment(closure);
        environment.define("this", instance);
        return new LoxFunction(declaration, environment, isInitializer);
    }
}