package com.nomoid.jlox;

class RuntimeError extends RuntimeException {
    // SerialVersionUID to suppress warning
    private static final long serialVersionUID = 1L;
    
    final Token token;

    RuntimeError(Token token, String message) {
        super(message);
        this.token = token;
    }
}