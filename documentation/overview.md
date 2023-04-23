# Gambit

Gambit is a programming language specifically designed for developing games. Alongside typical imperative features, it provides a declarative features for defining relationships between game objects. The core of this feature set are [Entities & Properties](#entities--properties)

## Entities & Properties

Entities represent types of game objects, such as cards, board pieces, or players. Properties define relationships between entities.

### Entity Declaration

To declare an entity, use the `entity` keyword followed by the entity name. e.g.

```gambit
entity Card
entity Player
```

### Property Declaration

Properties are declared by listing the related entity type(s) and the property name in square brackets `[]`. They may be listed in any order. All properties also have a [kind](#property-kinds) and a type, which are included before the brackets. e.g.

```gambit
fixed bool [Checker is_queen]
state bool [Checker is_placed_on GridSquare]
```

As shown, properties can be tied to one or more entities. For example, in a game with `Checker` and `GridSquare` entities, you could create a `bool [Checker is_placed_on GridSquare]` property. This would give each `Checker` in the game a boolean `is_placed_on` value for every `GridSquare` in the game.

Properties can also have default values. e.g.

```gambit
fixed bool [Checker is_queen] = false
```

If you want to refer to the entities related by the property in the property's value, you can put identifiers for the entities in the square brackets. e.g.

```gambit
trait bool [Player a matches_colour_with Player b] = a.colour == b.colour
```

> 🚩 Entities and properties are comparable to [classes](<https://en.wikipedia.org/wiki/Class_(computer_programming)>) and [attributes](<https://en.wikipedia.org/wiki/Class_(computer_programming)#Structure>) in other programming languages, but are not the same. Gambit entities do not use inheritance, methods, or other common OOP features. The significant difference is that in Gambit properties can be tied to one or more entities, unlike regular OOP attributes, which are inherently tied to one class.
>
> Entities and properties are ideal for implementing game objects (e.g. cards, game pieces, and players), as well as rules that relate to them. However, they may not be suitable for all cases where you might use a class in other languages.

### Property Kinds

There are three kinds of property: fixed, trait, and state. Each works slightly differently.

| Property | Description                                                | Example                              |                                                                     |
| -------- | ---------------------------------------------------------- | ------------------------------------ | ------------------------------------------------------------------- |
| Fixed    | A constant value that is known at compile time.            | the suit of a card                   | `fixed Suit [Card suit]`                                            |
| Trait    | A value that is derived formulaically from the game state. | if a chess piece can move diagonally | `trait bool [Piece p can_move_diagonal] = p.piece == BISHOP\|QUEEN` |
| State    | A value that changes during the game.                      | how many coins the player has        | `state int [Player coins] = 0`                                      |

> 🚩 All of a game's state properties together comprise the 'game state'.

## Selectors

Selectors select all entities with specific values for their properties. For example, all hearts in a deck of playing cards, or all players with 10 or more coins.

```gambit
Card<suit: HEART>
Player<tokens >= 10>
```

Boolean properties can be used as follows to select entities where the property is `true`.

```gambit
Card<in_play>
```

A selector is 'static' if it selects instances using only fixed properties, otherwise, it is 'dynamic'.

## Functions & Procedures

In Gambit, functions and procedures have are same, except for one key difference; functions must be "mathematically pure" and cannot change the game state, whereas procedures can change the game state. Functions are a subtype of procedures. You can imagine functions as 'read-only' procedures.

### Procedure Declaration

Procedures are declared without any keyword. e.g.

```gambit
main() {
	...
}
```

### Function Declaration

Functions are declared with the `fn` keyword.

```gambit
fn possible_moves(Player player) {
	...
}
```

Functions can also be declared using an arrow syntax. In this case, the function is defined using exactly one expression statement, the value of which will be returned by the function. e.g.

```gambit
fn can_win(Player player) => player.coins > 20
```

### Selector Parameters

When defining a parameter to a function or procedure, you may specify a [selector](#selectors) instead of a type.

### Procedure Overloads

Functions and procedures can be overloaded. Overloads must have the same return type, but must have different parameters / parameter types.

When a procedure is called, if multiple overloads match the arguments given, the overload with the most specific selectors will be executed.

```gambit
fn point_value(Card card)                      => 2
fn point_value(Card<suit: ACE> card)           => 2 + card.tokens
fn point_value(Card<suit: ACE|SPADES> card)    => 11
fn point_value(Card<suit: HEARTS, face: !ACE>) => 3
```

It is a compile-time error to have call to a procedure where the parameters match multiple overloads of the same specificity

```gambit
fn point_value(Card card) => 1

// These two functions have the same specificity
fn point_value(Card<value: ACE> card) => 4
fn point_value(Card<suit: SPADE> card) => 2

main() {
	score = point_value(Card<value: ACE, suit: SPADE>) // ERROR: It is ambiguous which overload of `point_value` should be called.
}
```

If, however, no ambiguous calls are identified at compile-time, both overloads may co-exist.

```gambit
fn point_value(Card card) => 1

// These two functions have the same specificity
fn point_value(Card<value: ACE> card) => 4
fn point_value(Card<suit: SPADE> card) => 2

main() {
	score = point_value(card<value: ACE, suit: HEART>)
	      + point_value(card<value:   3, suit: SPADE>)
}
```

### Method call syntax

If a procedure may be called the typical way.

```gambit
f(a, b, c)
```

However may also be called using the 'method syntax', where the first argument proceeds the function name.

```gambit
a:f(b, c)
```

## Types

### Primitive types

| Type        | Description                                                                     |
| ----------- | ------------------------------------------------------------------------------- |
| `Number`    | Any numerical value.                                                            |
| `Integer`   | A whole number value. Is a sub-type of `Number`.                                |
| `Boolean`   | `true` or `false`.                                                              |
| `String`    | Any string value.                                                               |
| `Procedure` | A procedure.                                                                    |
| `Function`  | A procedure that has no effect on the game state. Is a sub-type of `Procedure`. |

### Enum types

> TODO: Write up

### List types

> TODO: Write up
> Almost definitely need some notion of ordered and unordered lists?

### Optional types

Every type has a corresponding 'optional type', which can be a value of that type or `none`. It's syntax is `Type?`.

### Union types

Multiple types can be combined into one by creating their union. The original types will be considered subtypes of the union. The syntax is `TypeA | Type B`.

> ! How should this interact with multi-types?? Presumably something like `A!|B` is valid, and means you can be either one or more of A, or just one of B? So `A!|B` is not equal to `(A|B)!`. Though, confusingly, `A!?|B` is equal to `(A!|B)?`

### Multi-types

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

## Notation

In Gambit, it is possible to define notations for certain types of value. Notations are recognised using regular expressions and parsed using functions. This feature is particularly useful in cases where it is inconvenient or impractical to write out an entire structure literal by hand, but a nameless constructor would be too difficult to read.

### Creating a Notation

To create a notation, you need to provide Gambit with a regular expression and a function. If a match to the regular expression is found inside single quotes, Gambit will call the function with the matched string as its parameter.

For example, let's say we want to create a notation for [Magic the Gathering](https://mtg.fandom.com/wiki/Magic:_The_Gathering) style mana cost. We can define the following struct to represent the mana cost.

```gambit
struct ManaCost {
	Integer red = 0
	Integer black = 0
	Integer green = 0
	Integer blue = 0
	Integer white = 0
	Integer colourless = 0
}
```

We can then define a notation for this struct using the regular expression `[RBGUW]*(\d*)` and a function that maps the matched string to the fields of the `ManaCost` struct.

In this notation, `[RBGUW]*` matches any combination of the letters R, B, G, U, and W (representing the five different colors of mana), and `(\d*)` matches any number of digits (representing the amount of colorless mana required). The notation function maps the matched strings to the fields of the `ManaCost` struct.

```gambit
notation ManaCost ([RBGUW]*)(%d*) => match, colour, colourless {
    value.colourless = as_integer(colourless)
    for c in colour {
        match c {
            "R": value.red++
            "B": value.black++
            "G": value.green++
            "U": value.blue++
            "W": value.white++
        }
    }
}
```

## Using Notations

Once you have defined a notation, you can use it in your code by enclosing the notation in single quotes. For example, to create a `ManaCost` object with two red and one green mana, you can use the notation `'RRG'`:

### Ambiguity

In cases where a notated value matches multiple possible notations, Gambit will usually be able to infer the correct type from context. However, if it cannot infer a single type, an error will be thrown.
