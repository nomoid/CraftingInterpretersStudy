package com.nomoid.jlox;

import java.util.List;

class LoxFunction implements LoxCallable {

    private final Declaration declaration;
    private final Environment closure;

    LoxFunction(Stmt.Function declaration, Environment closure) {
        this.closure = closure;
        this.declaration = new FunctionDeclaration(declaration);
    }

    LoxFunction(Expr.Lambda declaration, Environment closure) {
        this.closure = closure;
        this.declaration = new LambdaDeclaration(declaration);
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
            return returnValue.value;
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
}