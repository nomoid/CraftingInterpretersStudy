1. We can solve this by making a constant cache map that is used during
compilation to store duplicate constants.
2. Challenge problem skipped.
3. I think it may be a good idea to at least report a warning when compiling
a script if a variable is never declared, to increase the chance that a mistake
like a typo would be caught before running the program. This would be even more
helpful if it could be "turned off" in some way, so that it's configurable by
the user.