package com.nomoid.jlox;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

class LoxClass extends LoxInstance implements LoxCallable, LoxClasslike{
    final String name;
    final LoxClass superclass;
    private final Map<String, LoxFunction> methods;
    private final Map<String, LoxFunction> getters;
    static final LoxClasslike none = new LoxClasslike(){
        
        @Override
        public String name() {
            return "None";
        }
    
        @Override
        public LoxFunction findMethod(String name) {
            return null;
        }

        @Override
        public LoxFunction findGetter(String name) {
            return null;
        }
    };

    LoxClass(String name, LoxClass superclass,
            Map<String, LoxFunction> methods,
            Map<String, LoxFunction> statics,
            Map<String, LoxFunction> getters) {
        super(new LoxClass(name, statics));
        this.name = name;
        this.methods = methods;
        this.getters = getters;
        this.superclass = superclass;
    }

    private LoxClass(String name, Map<String, LoxFunction> statics) {
        super(none);
        this.name = name + " metaclass";
        this.methods = statics;
        this.getters = new HashMap<>();
        this.superclass = null;
    }

    @Override
    public String name() {
        return name;
    }

    @Override
    public String toString() {
        return name();
    }

    @Override
    public Object call(Interpreter interpreter, List<Object> arguments) {
        LoxInstance instance = new LoxInstance(this);
        LoxFunction initializer = findMethod(Interpreter.INIT_STRING);
        if (initializer != null) {
            initializer.bind(instance).call(interpreter, arguments);
        }
        return instance;
    }

    @Override
    public int arity() {
        LoxFunction initializer = findMethod(Interpreter.INIT_STRING);
        if (initializer == null) {
            return 0;
        }
        return initializer.arity();
    }

    @Override
    public LoxFunction findMethod(String name) {
        if (methods.containsKey(name)) {
            return methods.get(name);
        }

        if (superclass != null) {
            return superclass.findMethod(name);
        }

        return null;
    }

    @Override
    public LoxFunction findGetter(String name) {
        if (getters.containsKey(name)) {
            return getters.get(name);
        }

        if (superclass != null) {
            return superclass.findGetter(name);
        }
        
        return null;
    }
    
}