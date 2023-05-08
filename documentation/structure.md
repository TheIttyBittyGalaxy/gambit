# Structure

> ⚙️ **Development Status:** Not implemented.

Structures are data types made up of one or more members. The `struct` keyword can be used to declare a new structure type. It is convention to name types in `PascalCase`.Each member must have a [pattern](pattern.md) and a name. It is convention to name members in `snake_case`. You may optionally specify a default value. If you don't, Gambit will specify a default.

Structure values are '[passed-by-value](https://stackoverflow.com/questions/373419/whats-the-difference-between-passing-by-reference-vs-passing-by-value)'.

Structures are ideal for representing different kinds of values that might appear in your game. For representing a specific type of game object, an [entity](entity.md) may be more fitting.

## Syntax

**Declare Structure**

<pre><code><strong>entity</strong> StructureName {
    MemberPattern member_name <em>= default value</em>
    ...
}</code></pre>

**Create Structure Value**

<pre><code>StructureName()
StructureName(member_name: member_value, ...)</code></pre>

## See more

-   [Types](type.md)
-   [Basic types](basic-types.md)
-   [Enums](enum.md)
-   [Entities](entity.md)
-   [Patterns](pattern.md)
