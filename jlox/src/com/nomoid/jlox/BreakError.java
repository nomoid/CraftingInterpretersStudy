package com.nomoid.jlox;

class BreakError extends RuntimeException {
    // SerialVersionUID to suppress warning
    private static final long serialVersionUID = 1L;
    
    final Token token;

    BreakError(Token token) {
        super();
        this.token = token;
    }
}