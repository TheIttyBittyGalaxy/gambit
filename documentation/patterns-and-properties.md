> 🚧 FIXME: These are just some rough notes I've made for my own reference. Tidy up and incorporate into the formal documentation at some point.

# Patterns

| Pattern       | Description                                                                                                | Syntax           |
| ------------- | ---------------------------------------------------------------------------------------------------------- | ---------------- |
| Named Pattern | Is the same as the given pattern, but also gives a name that refers to the value that matched that pattern | `Pattern : name` |
| Type\*        | Matches all values of that type.                                                                           |                  |
| Optional      | Matches all values in a pattern or `none`                                                                  | `Pattern?`       |

\* Types are implemented in the forms `NativeType`, `EnumType`, and `InvalidType`

## Not implemented

| Pattern  | Description                                                                            | Syntax              |
| -------- | -------------------------------------------------------------------------------------- | ------------------- |
| Set      | Matches any value in the given set.                                                    | `{a, b, c}`         |
| Value    | Matches the given value. Can be a variable. (Can be thought of as a set of one value). | -                   |
| Selector | Matches all instances of an entity that have the specified properties.                 | `Entity<a: 1,b: 2>` |

## Notes

As it currently stands, I don't _think_ there are any 'patterns' that match multiple values, e.g. `(foo1, foo2).bar`, and I think we can get away with not having them. We'll have to see how that evolves with the language though.

So far as I can tell, patterns also do not appear in expressions (or if they do, it's in very specific situations where there won't be an ambiguity as to if we're parsing a pattern or a value, e.g. `a is Type`). If patterns _can_ appear in expressions, some serious thought needs to be given as to how that should work!

## Overlaps

Given two patterns, A and B, their overlap will be one of the following

-   `COMPLETE`: One pattern completely encompasses the other (either because one is a subset, or because both patterns are equivalent)
-   `PARTIAL`: There are values that fall within both sets, but also values that fit in one but not the other.
-   `SEPARATE`: There are no values that fall within both sets.

# Properties

## Property Access

```gambit
foo               // VALID:   is just a value
foo.bar           // VALID:   indexes the `bar` property of `foo`
(foo).bar         // VALID:   indexes the `bar` property of `foo`
(foo1, foo2).bar  // VALID:   indexes the `bar` property of `(foo1, foo2)`
(foo1, foo2)      // INVALID: this syntax does not mean anything. must be followed by an index to have meaning
```

## State Properties

States identifiers can be overloaded, but cannot be overwritten. (i.e. once a state is declared, that's it, there's no modifying it - _but_ - you can declare multiple states with the same name). When you overload a state identifier, the following rules apply:

-   If the pattern of an overload has a `PARTIAL` overlap with the pattern of another overload in the same scope or an ancestor scope, this is a compile time 'ambiguity' error.

```gambit
state bool ({0,  1,  2,  3} a).foo
state bool ({0, -1, -2, -3} b).foo
// COMPILE TIME ERROR! Both overloads have patterns that partially overlap, meaning if ever you were to try and access 0.foo, it could be ambiguous as to which state named `foo` should apply.
```

-   If an attempt to resolve an access to a state yield multiple potential overloads, this is a compile time 'ambiguity' error.

```gambit
state int (RedCard card).tokens
state int (BlackCard card).tokens

Card card = ...
card.tokens
// COMPILE TIME ERROR! Ace of spades matches both patterns, so it is unclear which of the two `tokens` states could be accessed.
// Note: In this case, it would likely make the most sense to simply replace both states with:
// state int (Card card).tokens
```
