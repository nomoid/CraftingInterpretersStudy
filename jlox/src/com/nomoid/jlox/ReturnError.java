package com.nomoid.jlox;

class ReturnError extends RuntimeException {
    
    // SerialVersionUID to suppress warning
    private static final long serialVersionUID = 1L;
    final Token token;
    final Object value;
    
    ReturnError (Token token, Object value) {
        super(null, null, false, false);
        this.token = token;
        this.value = value;
    }
}