1.  Smalltalk uses named parameters for passing arguments into function calls. Therefore, if the number of parameters is incorrect, there would be no function associated with the name that is the list of parameters.
3.  The following program is valid:
    ```
    fun scope(a) {
        var a = "local";
    }
    ```
    In our current implementation of Lox, a function's parameters are in the same scope as is local variables. This matches behavior in other languages such as Java, where the scope of the parameter is the same as the local scope. This can be demonstrated more specifically in Java, because it disallows redeclaring local variables, so if you declare a local variable with the same name as a parameter, it results in a compile-time error.