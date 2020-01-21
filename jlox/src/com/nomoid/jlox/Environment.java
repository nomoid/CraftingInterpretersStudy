package com.nomoid.jlox;

import java.util.HashMap;
import java.util.Map;

class Environment {
    static final Object UNINITIALIZED = new Object();
    final Environment enclosing;
    private final Map<String, Object> values = new HashMap<>();

    Environment() {
        enclosing = null;
    }

    Environment(Environment enclosing) {
        this.enclosing = enclosing;
    }

    void define(String name, Object value) {
        values.put(name, value);
    }

    void assign(Token name, Object value) {
        if (values.containsKey(name.lexeme)) {
            values.put(name.lexeme, value);
            return;
        }

        if (enclosing != null) {
            enclosing.assign(name, value);
            return;
        }

        throw new RuntimeError(name,
            "Undefined variable '" + name.lexeme + "'.");
    }

    Object get(Token name) {
        if(values.containsKey(name.lexeme)) {
            Object value = values.get(name.lexeme);
            if (value == UNINITIALIZED) {
                throw new RuntimeError(name,
                    "Unintialized variable '" + name.lexeme + "'.");
            }
            return value;
        }

        if (enclosing != null) {
            return enclosing.get(name);
        }

        throw new RuntimeError(name,
            "Undefined variable '" + name.lexeme + "'.");
    }

    Object getAt(int distance, Token name) {
        Object value = ancestor(distance).values.get(name.lexeme);
        if (value == null) {
            // Unreachable code
            throw new InterpreterException(name,
                "Resolved name not found in environment.");
        }
        if (value == UNINITIALIZED) {
            throw new RuntimeError(name,
                "Unintialized variable '" + name.lexeme + "'.");
        }
        return value;
    }

    void assignAt(int distance, Token name, Object value) {
        Environment environment = ancestor(distance);
        if (!environment.values.containsKey(name.lexeme)) {
            throw new InterpreterException(name,
                "Resolved name not found in environment.");
        }
        environment.values.put(name.lexeme, value);
    }

    Environment ancestor(int distance) {
        Environment environment = this;
        for (int i = 0; i < distance; i++) {
            environment = environment.enclosing;
        }
        return environment;
    }
}