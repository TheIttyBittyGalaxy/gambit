> // FIXME: Work through these notes, getting rid of things that aren't useful any more, and turning things that are useful into proper documentation.

# Union types

Multiple types can be combined into one by creating their union. The original types will be considered subtypes of the union. The syntax is `TypeA | Type B`.

> ! How should this interact with multi-types?? Presumably something like `A!|B` is valid, and means you can be either one or more of A, or just one of B? So `A!|B` is not equal to `(A|B)!`. Though, confusingly, `A!?|B` is equal to `(A!|B)?`

# Multi-types

Every type has a corresponding 'multi type', which can be any combination of values of that type. It's syntax is `Type!`

For example, say you define different kinds of cards

```
DEFINE Suit { HEART, DIAMOND, SPADE, CLUB }
DEFINE Card {
	static Suit suit
	static Face face
}
```

In this case, cards can only be of one specific kind. If however we defined `suit` to be of type `Suit!`, each card could be any combination of suits. E.g. you could have the 'Ace of Hearts & Diamonds', or the 'Three of all the suits'.

Note that multi-types are not lists. You could not have the 'Ace of Spades and Spades again'. The `suit` may be multiple values, but each value in the domain can appear only once, and are in no particular order.

Values of type `Type!` must be of at least one value in that domain. e.g., with the card definition above, there could be no 'Ace of nothing'. However, all multi-type also have a corresponding optional multi-type, denoted as `Type!?`. Values of an optional multi-type may be of zero, one, or multiple values.

To create a multi-type value, you can combine multiple values with the `&` operator, e.g. `Spade&Heart`. A distinction between the `==` and `IS` operators is that the `==` seeks to make an exact match between multi-type values, and the `IS` operator only checks that every value on the right hand side is also on the left hand side. e.g.

```
Heart == Heart // true
Heart is Heart // true

Heart&Spade == Spade // false
Heart&Spade is Spade // true

Spade == Heart&Spade // false
Spade is Heart&Spade // false

Heart&Spade == Club&Spade // false
Heart&Spade is Club&Spade // false
```
