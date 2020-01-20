3.  For the following code block:
    ```
    var a = 1;
    {
        var a = a + 2;
        print a;
    }
    ```
    I expect the behavior is to print out the value `3`, but have the outer variable `a` remain assigned to the value `1`. I believe that users will expect this to follow the actual behavior, because the variable `a` on the inner scope is a completely different variable that just happens to have the same name.

Note: In addition to the challenges presented in this chapter, I also added support for "assignment by" operators, specifically `+=`, `-=`, `*=`, and `/=`, with potential to add more in the future.