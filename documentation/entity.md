# Entity

> ⚙️ **Development Status:** In the current compiler entities can be declared but not instantiated.

Entities are how Gambit defines different kinds of game object (e.g. cards, board pieces, or players). The `entity` keyword can be used to declare a new type of entity. It is convention to name types in `PascalCase`.

> // TODO: How are entities instantiated?

You can create instances of an entity type. Entity instances are '[passed-by-reference](https://stackoverflow.com/questions/373419/whats-the-difference-between-passing-by-reference-vs-passing-by-value)'.

Unlike classes or objects in other languages, entities do not have members. Use [properties](property.md) instead. For representing values with members that don't need to be treated as _objects_ (in the way C++ and other languages might), try [structures](structre.md) instead.

## Syntax

**Declare Entity**

<pre><code><strong>entity</strong> EntityName</code></pre>

## See more

-   [Properties](property.md)
-   [Entity selectors](entity-selector.md)
-   [Structures](structure.md)
-   [Types](type.md)
