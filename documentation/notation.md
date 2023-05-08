# Notation

> ⚙️ **Development Status:** Not implemented.

In essence, a notation allows you to define custom literals for specific types of values, usually a [structure](structure.md). A notation is defined with a regular expression and a function. Each time the compiler finds a match to the regular expression in single quotation marks, the function will be invoked, with all of the regular expression's matches passed as arguments into the function. The value returned by the function will be used in the place of the original notated value.

In cases where a notated value matches multiple possible notations, the compiler will attempt to infer the correct notation from context, using the return pattern of the notation function. If a single notation cannot be inferred, a compile time error is thrown.

## Syntax

**Declare Notation**

<pre><code><strong>notation</strong> ReturnPattern (regular_expression): match_variable_1, ... {
    ... function body ...
}</code></pre>

**Use Notation**

<pre><code>'match to regular expression'</code></pre>

## See more

-   [Guide: How to use notations](guide-how-to-use-notations.md)
-   [structures](structure.md)
