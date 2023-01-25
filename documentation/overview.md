# Entities

Entities are how you declare types of game object in Gambit.

Entity definitions declare the states, properties, and static values of a specific type of entity. These are known as the entity's 'fields'. An instance of an entity will have all of these fields. When defining an entity, you may also define it's signature - a shorthand way of referring to instances of that entity that match certain values.

'Additive definitions' allow you to declare additional fields for instances of an entity that have static fields that match certain values.

'Entity selectors' allow you select all instances of an entity type whose fields match certain values.

**Example**

```gambit
enum Face { ACE, 2, 3, 4, 5, 6, 7, 8, 9, JACK, QUEEN, KING }
enum Suit { HEART, DIAMOND, CLUB, SPADE }

entity Card[suit, face] {
	static Suit suit
	static Face face
	state int tokens: 0
	property bool is_powerful: tokens >= 3 or face == JACK|QUEEN|KING
}
```

> 🚩 Entities are _comparable_ to [classes](<https://en.wikipedia.org/wiki/Class_(computer_programming)>) in other programming languages, but are not the same thing. They do not use inheritance, methods, or other common OOP features. They are good for implementing game objects such as cards, game pieces, and players, but may not be suitable in all cases where you might use a class in other languages.

## Entity fields

There are three kinds of field a entity can have, each with different behaviours.

| field    | description                                                                                                                          | example                                          |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------ |
| Static   | Static fields are constant values that are known at compile time, e.g. the suit of a card.                                           | `static Suit suit`                               |
| State    | States are values that can change during the game, e.g. how many coin tokens the player has. They must defined with a default value. | `int tokens: 0`                                  |
| Property | Properties are values derived from other fields, e.g. if a chess piece can move diagonally. They must defined with a pure expression | `bool can_move_diagonal: piece == BISHOP\|QUEEN` |

> 🚩 All state fields across all entity instances comprise the 'game state'.

## Entity definitions

### Base definitions

The definition where an entity is 'first' declared is called the 'base definition'. This does not actually have to be the first definition the compiler sees, however it will be treated as the base definition that all other 'additive definitions' build on top of. For this reason, each type of entity must have exactly one base definition.

Base definitions are indicated using the `entity` keyword.

```gambit
entity Card {
	static Suit suit
	static Face face
}
```

Base definitions may optionally specify a signature for the entity. This is a comma separated list of field names in-between `[` `]` brackets. The signature is written after the entity's name.

```gambit
entity Card[suit, face] {
	static Suit suit
	static Face face
}
```

### Additive definitions

Once a entity has been defined, you may also define additional fields in a separate 'additive definition'. You may also use additive definitions to provide values to fields that were defined in other definitions.

Additive definitions are indicated using the `extend` keyword.

```gambit
entity Card[suit, face] {
	static Suit suit
	static Face face
}

extend Card {
	state Boolean activated
}
```

You can create additive definitions that only apply to instances of the entity that match a specific static selector. In this case, the selector goes after the entity's name. This is the most useful way to use additive definitions.

```gambit
entity Card[face, suit] {
	static Suit suit
	static Face face
}

extend Card[face: ACE|JACK|QUEEN|KING] { // This selector matches any picture cards
	state int tokens: 0
	static int tokens_need_to_win: 3
}

extend Card[ACE, SPADES] { // This selector uses the entity's signature to match the Ace of Spades.
	static int tokens_need_to_win: 2
}
```

### Entity definition scoping

All entity definitions (base and additive) are scoped to the namespace they are written in. An entity type is scoped to the same namespace as the base definition.

In order for a statement or expression to access a field of an entity, it must be in the same scope as the definition where that field was defined (or any nested scope).

```gambit
entity Foo {
	state int bar
}

namespace GameExpansion {
	extend Foo {
		state int super_bar
	}

	// Can access Foo.super_bar here
}

// Cannot access Foo.super_bar here
```

Because entity fields are scoped in this way, it is possible for an entity to have multiple fields with the same name, but that exist in separate scopes.

```gambit
entity Foo { ... }

namespace RubyExpansion {
	extend Foo {
		static string power
	}
}

namespace EmeraldExpansion {
	extend Foo {
		state int power: 0
	}
}
```

## Entity signatures

An entity signature is a pattern that matches instances of an entity based on the values of it's fields. It is a way to specify the values specific fields of an entity without having to them them.

For example...

```gambit
define entity Card[suit, face] {
	static Suit suit
	static Face face
}
```

...allows the pattern `Card[suit: SPADES, face: ACE]` to be rewritten as `Card[SPADES, ACE]`. If the type can be inferred from context, it can be shortened even further to just `[SPADES, ACE]`.

It is legal to provide partial signatures - just so long as it is unambiguous what is meant by them.

```gambit
Card[CLUB] // Matches any clubs
Card[ACE] // Matches any aces
```

In this case, is totally unambiguous if the value provided is the `suit` or the `face`, as those two fields have no overlap in their domain of values. If they did, the syntax would be invalid.

## Instantiating entities

In order for an instance of an entity to be created, all of it's static fields must be specified.

> TODO: Figure out a good syntax for this

# Selectors

During your game, you can select all entity instances that have specific values. e.g. all of the hearts in a deck of cards, all players with 10 or more coin tokens, etc.

```gambit
Card[suit: HEART]
Player[tokens >= 10]
```

You can add as many selectors as you like to select entities under specific circumstances.

```gambit
Card[in_play][face: ACE][tokens >= 3]
```

You can also select entities using their signature.

```gambit
Card[ACE, SPADE]
```

A selector is said to be a 'static selector' if it selects instances based only on their static fields. A selector is said to be a 'dynamic selector' if it selects instances using one or more state or property fields\*.

> \* Technically, a property is considered to be a 'static property' if the expression used to define it is also static. Static properties do not cause a selector to become a dynamic one, and can be used in a static one.

# Functions & Procedures

Functions and procedures in Gambit are two different things. The only difference between them is that functions must be 'mathematically pure', or in other words, cannot change the game state. Functions are considered to be a subtype of Procedures, and so all functions are also procedures. You can imagined functions as 'read-only' procedures.

## Method call syntax

If a function or procedure may be called the typical way.

```gambit
f(a, b, c)
```

However may also be called using the 'method syntax', where the first argument proceeds the function name.

a:f(b, c)

## Selectors as parameters

When defining a function or procedure (henceforth 'procedures') parameter, you may specify it's type, but you may also specify a static selector. Procedures can be overloaded. If multiple overloads match the same set of parameter types, the overloads will be ranked by the specificity of their selectors, and the first to match the given parameter will be executed.

```gambit
fn point_value(Card card)                 => 2
fn point_value(Card[ACE] card)            => 2 + card.tokens
fn point_value(Card[ACE, SPADES] card)    => 11
fn point_value(Card[HEARTS][face != ACE]) => 3
```

It is a compile-time error to have call to a procedure where the parameters match multiple overloads of the same specificity

```gambit
fn point_value(Card card) => 2

// These two functions have the same specificity
fn point_value(Card[SPADE] card) => 1
fn point_value(Card[ACE] card) => 4

main() {
	score = point_value([ACE, SPADE]) // ERROR: It is ambiguous which overload of `point_value` should be called.
}
```

If, however, no ambiguous calls are identified at compile-time, both overloads may co-exist.

```gambit
fn point_value(Card card) => 2

// These two functions have the same specificity
fn point_value(Card[SPADE] card) => 1
fn point_value(Card[ACE] card) => 4

main() {
	score = point_value([ACE, HEART])
	      + point_value([  3, SPADE])
}
```

> TODO: Is only allowing static selectors the right choice? On one hand, it allows us to do comprehensive static analysis. This is most useful for preventing situations where two overloads with the same specificity match a given set of arguments, as we will be able to detect if this occurs at compile time. On the other hand, in some games, differing behaviours may be purely the result of non-static fields. In this case, currently the programmer is forced to place any logic that interacts with these dynamic fields in the function itself. This code is non expandable, _unless_ they use dynamic procedure fields to allow entities to specify their own logic. Maybe that's the right call, but maybe it comes with it's own issues?

# Types

## Primitive types

| Type        | Description                                                                     |
| ----------- | ------------------------------------------------------------------------------- |
| `Number`    | Any numerical value.                                                            |
| `Integer`   | A whole number value. Is a sub-type of `Number`.                                |
| `Boolean`   | `true` or `false`.                                                              |
| `String`    | Any string value.                                                               |
| `Procedure` | A procedure.                                                                    |
| `Function`  | A procedure that has no effect on the game state. Is a sub-type of `Procedure`. |

## Enum types

> TODO: Write up

## List types

> TODO: Write up
> Almost definitely need some notion of ordered and unordered lists?

## Optional types

Every type has a corresponding 'optional type', which can be a value of that type or `none`. It's syntax is `Type?`.

## Union types

Multiple types can be combined into one by creating their union. The original types will be considered subtypes of the union. The syntax is `TypeA | Type B`.

> ! How should this interact with multi-types?? Presumably something like `A!|B` is valid, and means you can be either one or more of A, or just one of B? So `A!|B` is not equal to `(A|B)!`. Though, confusingly, `A!?|B` is equal to `(A!|B)?`

## Multi-types

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

## First class functions and procedures

Functions and procedures can be first class. This also means they can be the fields of entities. For example, say that some cards in your game have an 'activate' ability, that could be implemented something like this.

```gambit
define entity Card {
	static Procedure(Player user)? activate_ability
	state bool activated = false
	...
}

procedure activate_card(Player player, Card card) {
	IF card HAS activate_ability AND NOT card.activated {
		card.activate_ability(player)
		card.activated = true
	}
}

procedure gain_two_money(Player player) {
	player.money += 2
}

atm = entity Card {
	activate_ability = gain_two_money
}

jackpot = entity Card {
	procedure activate_ability(user) {
		card.user.money += roll(6) + roll(6)
	}
}
```

# Notation

Often, it can be useful to specify a notion for certain kinds of values that may appear in your game. For example, in [Magic the Gathering](https://mtg.fandom.com/wiki/Magic:_The_Gathering), cards have a 'mana cost'. There are 5 different 'colours' of mana, and in even the most basic cases, a card may specify a cost of any amount of each specific kind of mana, and/or a certain amount of mana of any colour. e.g. the card 'Abundance' require you may 2 green mana and then also 2 mana of any colour. A notation such as `GG2` would be more convenient that having to type out all the names and fields of a struct by hand, while remaining more readable than a nameless constructor.

Gambit allows you to define a notations for certain kinds of values. You provide Gambit with a regular expression and a function. Then, if in an expression Gambit matches a `'` followed by a match to your regular expression, it will run your function with the matched notation as it's parameter.

To parse our simple MTG mana cost notation, we could do something like this

```gambit
structure ManaCost {
	Integer red = 0
	Integer black = 0
	Integer green = 0
	Integer blue = 0
	Integer white = 0
	Integer colourless = 0
}

notation ManaCost ([RBGUW]*)(%d*) -> match, colour, colourless {
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

...

procedure do_thing(ManaCost cost) {
	 return cost == 'RRG
}
```

## Ambiguity

When a notated value matches multiple possible notations, usually this ambiguity is resolved using type inference. e.g. maybe `%2` could annotate a mama cost, or it could annotate a power and toughness. Usually, Gambit can tell from context. In cases where it can't however, this will throw an error.
