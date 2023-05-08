# Optional pattern

> ⚙️ **Development Status:** Implemented.

> // FIXME: This doesn't feel quite like the right name for it? Somehow the meaning of 'optional type' is intuitive, but somehow 'optional pattern' makes it sound like you get to choose if there's a pattern, rather than a pattern that allows you to optionally specify a value.

Making an pattern optional will create a new pattern, which match all values in the original pattern and `none`. Patterns that already match `none` cannot be made optional.

## Syntax

**Make a pattern optional**

<pre><code>pattern?</code></pre>

## See more

-   [Patterns](pattern.md)
-   [Types](type.md)
