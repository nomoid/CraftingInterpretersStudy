1.  The following grammar
    ```
    expr → expr ( "(" ( expr ( "," expr )* )? ")" | "." IDENTIFIER )*
        | IDENTIFIER
        | NUMBER
    ```
    is equivalent to the following grammar without syntactic sugar
    ```
    expr → expr expr2
    expr → IDENTIFIER
    expr → NUMBER

    expr2 -> "(" expr3 ")"
    expr2 -> "()"
    expr2 -> "." IDENTIFIER

    expr3 -> expr
    expr3 -> expr3 "," expr
    ```
    This kind of grammar encodes function calling in a C-like language.
2.  A corresponding pattern in a functional language would be a record type with functions as each of the parameters e.g. `type FeatureBox = { feature1: int -> int, feature2: () -> string }`. In order to add a new type `T` that supports `FeatureBox`, you would only need to write a function `f` with type `T -> FeatureBox`. Then, if anyone needs to call a feature (say `feature1`) on some instance `t` of `T`, they would call `(f t).feature1(input)`. There could also be some central function `g` that takes any object `t` and finds the corresponding `f` for it, which would make `(g t).feature1(input)` work for any supported `t`.