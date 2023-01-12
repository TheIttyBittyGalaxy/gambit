# 1. Entities

A entity is 'a thing that can exist' in your game. For example, if your game is played with a regular deck of cards, a `Card` is a entity in your game. Entities must be defined, where they are defined with a list of fields. Each field must have a name and a data type, will be either a static field, a property, or a state.

Once a entity has been defined, you may create instances of it. If your game were played with a deck of cards, there would be 1 card definition and 52 card instances.

Entities are _similar_ to classes in other programming languages, but don't behave in the same way, and are designed for implementing things that are specified by the game's rules. e.g. you would implement an important game piece as a entity, but you wouldn't have a 'GamePieceManager' like you might in conventional object-oriented languages.

## 1.1 Entity definitions

### 1.1.1 Base definitions

Let's set we wanted to define a regular set of playing cards. We could do that something like this.

```gambit
enum Face { ACE, 2, 3, 4, 5, 6, 7, 8, 9, JACK, QUEEN, KING } // `Suit` and `Face` are both enums (see more in the 'types' section of this document)
enum Suit { HEART, DIAMOND, CLUB, SPADE }

define entity Card[suit, face] {
	static Suit suit
	static Face face
}
```

### 1.1.2 Additive definitions

Once a entity has been defined, you may also define additional fields in separate declarations. This is particularly useful in situations where you want to program extensions to the game in a separate file, without having to make significant edits to the 'core' game.

Additive definitions are the same as base definitions, but without the `define` keyword. Additive definition may not define the entity's signature.

```gambit
define entity Card[suit, face] {
	static Suit suit
	static Face face
}

entity Card {
	state Boolean activated
}
```

You may also use additive definitions to define the values of fields that were defined without values in other definitions. This is most useful when using selective definitions.

### 1.1.3 Selective definitions

You can create additive definitions that only apply to instances that match a given static selector. These selectors may define additional fields, or define the values of fields that were already defined elsewhere.

Let's say that in our game, played with a regular set of playing cards, the player has the ability to add tokens to picture cards. We could define that as follows.

```gambit
entity Card {
	static Suit suit
	static Face face
}

entity Card[face: ACE|JACK|QUEEN|KING] {
	state Integer tokens: 0
}
```

We could even add an additional rule that states that the Ace of Spades has a default value of 2 tokens.

```gambit
entity Card[ACE, SPADE] {
	tokens: 2
}
```

### 1.1.4 Scoped definitions

Additive definitions of a entity are scoped, meaning the additional fields can only be accessed elsewhere in the same scope. Different scopes may specify different fields that still have the same name.

```gambit
scope sapphire_expansion {
	entity Card {
		state Boolean super_shiny
		state Integer tokens // Can only be accessed in the 'sapphire_expansion' scope
	}
}

scope ruby_expansion {
	entity Card {
		state Integer tokens // Can only be access in the 'ruby_expansion' scope
	}
}
```

## 1.2 Entity signatures

Given the definition below, instances of a class would have to specified with the long-hand named syntax `Card[suit: SPADES, face: ACE]`.

```gambit
define entity Card {
	static Suit suit
	static Face face
}
```

This can be a bit cumbersome. However, in the base definition of a entity you can specify a 'signature' for the entity. This provides a syntax for supplying specific fields without having to name them. Given the following definition...

```gambit
define entity Card[suit, face] {
	static Suit suit
	static Face face
}
```

...the Ace of Spades can now be written as `Card[SPADES, ACE]`. If the value of the value literal can be inferred, it can be shortened even further to just `[SPADES, ACE]`.

### 1.2.1 Partial signatures

Partial signatures can also be provided in the case where it is unambiguous what is meant by them.

```gambit
Card[CLUB] // Matches any clubs
Card[ACE] // Matches any aces
```

In the case of playing cards, if you only provide a single value to the signature, it is totally unambiguous as if to the `suit` or the `face` has been provided, because those two fields have no overlap in their domain of values. If they did, the syntax would be invalid.

## 1.3 Entity fields

There are three kinds of field a entity can have, each with different behaviours.

### 1.3.1 Static fields

Static fields constant values that are known at compile time. An example might be the suit of a card.

```gambit
static Suit suit
```

### 1.3.2 State fields

State fields are fields that can change during the game. State fields must be defined with a default value. An example might be how many coin tokens the player has.

All state fields across all entity instances comprise the 'game state'.

```gambit
int tokens = 0
```

### 1.3.3 Property fields

A property of a entity is a field that is derived from other parts of the game state. An example would be if a king is in check in chess. They are defined with a pure expression, which is scoped to the entity it is a member of.

```gambit
static int max_health: 20 // Static field
int health: 1 // State field
num health_percentage => health / max_health // Property field
```

## 1.4 Creating instances

We can create a card with the following syntax. In order for an instance to be created, all of it's static fields must be specified.

```gambit
Card ace_of_spades = [SPADES, ACE]
```

To do these one by one for a full deck of cards would be tedious, and so we can use loops instead.

```gambit
List<Card> deck
for suit in Suit {
	for face in Face {
		deck.insert([suit, face])
	}
}
```

Which in Gambit can be reduced to

```gambit
List<Card> deck = for suit, face in Suit, Face => [suit, face]
```

> TODO: This isn't the best looking code I've ever seen. Perhaps study some functional languages to see how they might to something like this?

# 2. Selectors

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

# 3. Functions & Procedures

Functions and procedures in Gambit are two different things. The only difference between them is that functions must be 'mathematically pure', or in other words, cannot change the game state. Functions are considered to be a subtype of Procedures, and so all functions are also procedures. You can imagined functions as 'read-only' procedures.

## 3.1 Method call syntax

If a function or procedure may be called the typical way.

```gambit
f(a, b, c)
```

However may also be called using the 'method syntax', where the first argument proceeds the function name.

a:f(b, c)

## 3.2 Selectors as parameters

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

# 4. Types

## 4.1 Primitive types

| Type        | Description                                                                     |
| ----------- | ------------------------------------------------------------------------------- |
| `Number`    | Any numerical value.                                                            |
| `Integer`   | A whole number value. Is a sub-type of `Number`.                                |
| `Boolean`   | `true` or `false`.                                                              |
| `String`    | Any string value.                                                               |
| `Procedure` | A procedure.                                                                    |
| `Function`  | A procedure that has no effect on the game state. Is a sub-type of `Procedure`. |

## 4.2 Enum types

> TODO: Write up

## 4.3 List types

> TODO: Write up
> Almost definitely need some notion of ordered and unordered lists?

## 4.4 Optional types

Every type has a corresponding 'optional type', which can be a value of that type or `none`. It's syntax is `Type?`.

## 4.5 Union types

Multiple types can be combined into one by creating their union. The original types will be considered subtypes of the union. The syntax is `TypeA | Type B`.

> ! How should this interact with multi-types?? Presumably something like `A!|B` is valid, and means you can be either one or more of A, or just one of B? So `A!|B` is not equal to `(A|B)!`. Though, confusingly, `A!?|B` is equal to `(A!|B)?`

## 4.6 Multi-types

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

## 4.7 First class functions and procedures

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

# 5. Notation

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

## 5.1 Ambiguity

When a notated value matches multiple possible notations, usually this ambiguity is resolved using type inference. e.g. maybe `%2` could annotate a mama cost, or it could annotate a power and toughness. Usually, Gambit can tell from context. In cases where it can't however, this will throw an error.
