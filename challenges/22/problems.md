1. One way of more efficiently looking up variables is using a sort of
hash table mechanism for looking up the variable based on its name. However, I
suspect this might not actually be more efficient, because normally there are
not too many variables for this method to be a timesaver, since it has better
asymptotic behavior, but not necessarily better behavior for few local
variables. The reason why this is probably not worth pursuing is that this
would not actually have an impact on run-time behavior, since it is all
happenning at compile time.
2. I would make this statement an error, since it doesn't make sense to
assign a variable to a variable of the same name. If such an effect was
desired for scoping purposes, the user can add an additional temporary variable
to achieve the desired effect.
3. Implemented in codebase. Local consts are compile-time checked, while
global consts are runtime-checked.
4. Implemented in codebase. Implementation similar to implementation for
long constants - new instructions are used for local variables >= 256.