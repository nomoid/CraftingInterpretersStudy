1.  When first-class functions and dynamic dispatch is supported, then branching can be implemented in the following manner:
    ```
    var x = condition();
    var ifTrue() {
        // do something
    }
    var ifFalse() {
        // do something
    }
    x or ifTrue();
    x and ifFalse();
    ```
    A language that does this is Smalltalk, where the `Boolean` class has two methods: `ifTrue` and `ifFalse`, each of which take afunction. When the boolean has value `True`, the function passed to `ifTrue` is executed, whereas when the boolean has value `False`, the function passed to `ifFalse` is executed.
2.  Looping can be implemented with recursion, provided the interpreter supports tail-call optimization. Tail-call optimization makes recursion fast if the call to the recursive function is the final call a function makes before returning. The optimization works by throwing away the call stack information of the previous call, which is necessary to implement looping, because otherwise the code would frequently run into issues of stack overflow. One language that uses tail-call optimization is Scheme, meaning that a factorial function defined as the follwing utilizes tail-call optimization (code from [here](https://stackoverflow.com/questions/310974/what-is-tail-call-optimization)):
    ```scheme
    (define (fact x)
        (define (fact-tail x accum)
            (if (= x 0) accum
                (fact-tail (- x 1) (* x accum))))
        (fact-tail x 1))
    ```