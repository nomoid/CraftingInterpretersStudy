1.
    - `1 * 2 + 3`
        - `OP_CONSTANT 1`
        - `OP_CONSTANT 2`
        - `OP_MULTIPLY`
        - `OP_CONSTANT 3`
        - `OP_ADD`
    - `1 + 2 * 3`
        - `OP_CONSTANT 1`
        - `OP_CONSTANT 2`
        - `OP_CONSTANT 3`
        - `OP_MULTIPLY`
        - `OP_ADD`
    - `3 - 2 - 1`
        - `OP_CONSTANT 3`
        - `OP_CONSTANT 2`
        - `OP_SUBTRACT`
        - `OP_CONSTANT 1`
        - `OP_SUBTRACT`
    - `1 + 2 * 3 - 4 / -5`
        - `OP_CONSTANT 1`
        - `OP_CONSTANT 2`
        - `OP_CONSTANT 3`
        - `OP_MULTIPLY`
        - `OP_ADD`
        - `OP_CONSTANT 4`
        - `OP_CONSTANT 5`
        - `OP_NEGATE`
        - `OP_DIVIDE`
        - `OP_SUBTRACT`

2. `4 - 3 * -2`
    - Without `OP_NEGATE`
        - `OP_CONSTANT 4`
        - `OP_CONSTANT 3`
        - `OP_CONSTANT 0`
        - `OP_CONSTANT 2`
        - `OP_SUBTRACT`
        - `OP_MULTIPLY`
        - `OP_SUBTRACT`
    - Without `OP_SUBTRACT`
        - `OP_CONSTANT 4`
        - `OP_CONSTANT 3`
        - `OP_CONSTANT 2`
        - `OP_NEGATE`
        - `OP_MULTIPLY`
        - `OP_NEGATE`
        - `OP_ADD`

    In general, I think it makes sense to have both instructions, because not having both instructions makes it so that extra instructions are needed to deal with either negating and subtraction.
    Other redundant instructions I can think of including are
    `OP_INC` and `OP_DEC` for adding and subtracting by one, because I imagine those would be very common operations, so using an extra opcode slot to save one opcode each increment or decrement may be worth it.
3. The cost of dynamically growing the stack are that the amount of memory used can grow unbounded, but the benefit is that programs that need a larger stack will still work fine.