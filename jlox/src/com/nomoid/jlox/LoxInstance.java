package com.nomoid.jlox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

class LoxInstance {
    private LoxClasslike klass;
    private final Map<String, Object> fields = new HashMap<>();

    LoxInstance(LoxClasslike klass) {
        this.klass = klass;
    }

    @Override
    public String toString() {
        return klass.name() + " instance";
    }

    Object get(Interpreter interpreter, Token name) {
        if (fields.containsKey(name.lexeme)) {
            return fields.get(name.lexeme);
        }

        LoxFunction method = klass.findMethod(name.lexeme);
        if (method != null) {
            return method.bind(this);
        }

        LoxFunction getter = klass.findGetter(name.lexeme);
        if (getter != null) {
            return getter.bind(this).call(interpreter, List.of());
        }

        throw new RuntimeError(name,
            "Undefined property '" + name.lexeme + "'.");
    }

    void set(Token name, Object value) {
        fields.put(name.lexeme, value);
    }
}