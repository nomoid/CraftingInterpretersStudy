package com.nomoid.jlox;

class InterpreterException extends RuntimeException {
    // SerialVersionUID to suppress warning
    private static final long serialVersionUID = 1L;
    
    final Token token;

    InterpreterException(Token token, String message) {
        super(message);
        this.token = token;
    }
}