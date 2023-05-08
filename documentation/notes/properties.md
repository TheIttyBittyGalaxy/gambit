> // FIXME: Work through these notes, getting rid of things that aren't useful any more, and turning things that are useful into proper documentation.

> // NOTE: This is now largely out of date! The concept is _similar_ but features notable changes.

# Properties

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
