3.  The tradeoffs between these approaches depends on the view of the tradeoff between safety and power. A language that allows open access to object fields is more powerful, in that potentially more powerful abstractions can be made to work, above the original creator's intentions. However, this comes at a cost, which is that the language is less safe, especially if a field intended to be private is an implementation detail rather than part of a public API, which means that it could be changed at any time. Dynamic languages typically prefer free access, particularly because they believe in maximizing flexibility for their users (if the type system is already not inherently safe, it doesn't cost as much to introduce the unsafety of public fields to the language).