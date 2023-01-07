# 1. Constructs

A construct is 'a thing that can exist' in your game. For example, if your game is played with a regular deck of cards, a `Card` is a construct in your game. Constructs must be defined, where they are defined with a list of fields. Each field must have a name, a data type, and a sort (either `static`, `state`, or `property`). Once a construct has been defined, you may create instances of it. If your game were played with a deck of cards, there would be 1 card definition and 52 card instances.

Constructs are _similar_ to classes in other programming languages, but don't behave in the same way, and are designed for implementing things that are specified by the game's rules. e.g. you would implement an important game piece as a construct, but you wouldn't have a 'GamePeiceManager' like you might in conventional object-oritented lanaguges.

## 1.1 Construct definitions

### 1.1.1 Base definitions

Let's set we wanted to define a regular set of playing cards. We could do that something like this.

```gambit
enum Face { ACE, 2, 3, 4, 5, 6, 7, 8, 9, JACK, QUEEN, KING } // `Suit` and `Face` are both enums (see more in the 'types' section of this document)
enum Suit { HEART, DIAMOND, CLUB, SPADE }

define construct Card[suit, face] {
	static Suit suit
	static Face face
}
```

### 1.1.2 Additive definitions

Once a construct has been defined, you may also define additional fields in seperate declarations. This is particually useful in situations where you want to program extensions to the game in a seperate file, without having to make significant edits to the 'core' game.

Additive definitions are the same as base definitions, but without the `define` keyword. Additive definition may not define the construct's signature.

```gambit
define construct Card[suit, face] {
	static Suit suit
	static Face face
}

construct Card {
	state Boolean activated
}
```

### 1.1.3 Selective definitions

You can create additive definitions that only apply to a particular subsection of instances. Static fields are ones whose value are known at compile-time. You can select instances that have specific values of static field, and define additional fields for them.

Let's say that in our game, played with a regular set of playing cards, the player has the ability to add tokens to picture cards. We could define that as follows.

```gambit
construct Card {
	static Suit suit
	static Face face
}

construct Card[face: ACE|JACK|QUEEN|KING] {
	state Integer tokens: 0
}
```

### 1.1.4 Scoped definitions

Additive definitions of a construct are scoped, meaning the additional fields can only be accessed elsewhere in the same scope. Different scopes may specify differents fields that still have the same name.

```gambit
scope sapphire_expansion {
	construct Card {
		state Boolean super_shiny
		state Integer tokens // Can only be accessed in the 'sapphire_expansion' scope
	}
}

scope ruby_expansion {
	construct Card {
		state Integer tokens // Can only be access in the 'ruby_expansion' scope
	}
}
```

## 1.2 Construct signatures

Given the definition below, instances of a class would have to specified with the long-hand named syntax `Card[suit: SPADES, face: ACE]`.

```gambit
define construct Card {
	static Suit suit
	static Face face
}
```

This can be a bit cumbersome. However, in the base definition of a construct you can specify a 'signature' for the construct. This provides a syntax for supplying specific fields without having to name them. Given the following definition...

```gambit
define construct Card[suit, face] {
	static Suit suit
	static Face face
}
```

...the Ace of Spades can now be written as `Card[SPADES, ACE]`. If the value of the value literal can be inffered, it can be shortened even further to just `[SPADES, ACE]`.

### 1.2.1 Partial signatures

Partial signatures can also be provided in the case where it is umabigious what is meant by them.

```gambit
Card[CLUB] // Matches any clubs
Card[ACE] // Matches any aces
```

In the case of playing cards, if you only provide a single value to the signature, it is totally unambigous as if to the `suit` or the `face` has been provided, because those two fields have no overlap in their domain of values. If they did, the syntax would be invalid.

## 1.3 Construct field sorts

As well as having a data type each field has a 'sort'. This indicates what kind of property of the construct the field is.

| Sort         | Description                                      | e.g.                                 |
| ------------ | ------------------------------------------------ | ------------------------------------ |
| **static**   | A constant value that is known at compile time.  | the suit of a card.                  |
| **state**    | A value that can change during the game.         | how many coin tokens the player has. |
| **property** | A value that is calculated using the game state. | if a king is in check in chess.      |

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

> TODO: This isn't the best looking code I've ever seen. Perhaps study some functional langauges to see how they might to something like this?

# 2. Selectors

Property fields of constructs can be set using 'selectors'. A selector will match all instances that match a given pattern, and then specify it's properties accordingly. More selectors with more specific patterns override selectors with less specific ones (similar to other declarative languages such as CSS).

For example, let's say that in your game cards can accumulate tokens. A card becomes 'activated' either when it has at least 3 tokens, or just 1 token if it happens to be an ace. That could be implemented like this.

```gambit
define construct Card {
	static Suit suit
	static Face face
	state int tokens = 0
	property bool activated = false // The default value for the property if no selectors are matched
}

select Card[tokens >= 3] {
	activated = true
}

select Card[face: ACE, tokens >= 1] {
	activated = true
}
```

This this case the `Card[face: ACE, tokens >= 1]` selector overrides the `Card[tokens >= 3]` selector, which in turn overrides the base case specified in the `Card` definition.

> TODO: What should happen in the situation where there are multiple selectors that are both a) equal in specificity and b) have overlap in the values they can select? In CSS, the selector further down in the document takes precedence, but I don't think that is sufficient for our purposes, particually as the programmer has no control over the order in which files load.

## 2.1 Side note

It's worth noting that the example above can actually be written much simpler. Because properties can be set to pure expressions (expressions that do not affect the game state), you could rewrite the code as the following.

```
define construct Card {
	static Suit suit
	static Face face
	state int tokens = 0
	property bool activated = tokens >= 3 or (tokens >= 1 and face == ACE)
}
```

## 2.2 Example use case

The selector system is most advantageous in situations where different instances of a construct may behave in very specific and different ways. e.g. in [Disney© Villainous](https://disney-villainous.fandom.com), a card's 'kind' is _almost_ always a single static value, but in some cases can be changed by the abilities of other cards. e.g. Jafar can 'hypnotise' 'Hero' cards, at which point they become 'Ally' cards. That could look something like this.

```gambit
// In 'main.gambit'
enum CardKind { HERO, ALLY, ... }
define construct Card {
	property CardKind kind
	...
}

// In 'jaffar.gambit'

construct Card {
	state bool hypnotised = false
}

aladin = construct Card {
	kind = HERO
	....
}

select Card[hyponotised: true] {
	kind = ALLY
}
```

# 3. Functions & Procedures

Functions and procedures in Gambit are two different things. The only difference between them is that functions must be 'mathematically pure', or in other words, cannot change the game state. Functions are 'read-only' procedures.

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
> Almost definately need some notion of ordered and unordered lists?

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

Values of type `Type!` must be of at least one value in that domain. e.g., with the card definition above, there could be no 'Ace of nothing'. However, all mulitypes also have a corresponding optional multitype, denoted as `Type!?`. Values of an optional multitype may be of zero, one, or multiple values.

To create a multitype value, you can combine multiple values with the `&` operator, e.g. `Spade&Heart`. A distiction between the `==` and `IS` operators is that the `==` seeks to make an exact match between multitype values, and the `IS` operator only checks that every value on the righthand side is also on the lefthand side. e.g.

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

Functions and procedures can be first class. This also means they can be the fields of constructs. For example, say that some cards in your game have an 'activate' ability, that could be implemented something like this.

```gambit
define construct Card {
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

atm = construct Card {
	activate_ability = gain_two_money
}

jackpot = construct Card {
	procedure activate_ability(user) {
		card.user.money += roll(6) + roll(6)
	}
}
```

# 5. Notation

Often, it can be useful to specify a notion for certian kinds of values that may appear in your game. For example, in [Magic the Gathering](https://mtg.fandom.com/wiki/Magic:_The_Gathering), cards have a 'mana cost'. There are 5 different 'colours' of mana, and in even the most basic cases, a card may specifiy a cost of any amount of each specific kind of mana, and/or a certian amount of mana of any colour. e.g. the card 'Abundance' require you may 2 green mana and then also 2 mana of any colour. A notation such as `GG2` would be more convient that having to type out all the names and fields of a struct by hand, while remaining more readable than a nameless constructor.

Gambit allows you to define a notations for certian kinds of values. You provide Gambit with a regular expression and a function. Then, if in an expression Gambit matches a `'` followed by a match to your regular expression, it will run your function with the matched notation as it's parameter.

To parse our simpile MTG mana cost notation, we could do something like this

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

## 5.1 Ambigutity

When a notated value matches multiple possible notations, usually this ambigutity is resolved using type inference. e.g. maybe `%2` could annotate a mama cost, or it could annotate a power and thoughness. Usually, Gambit can tell from context. In cases where it can't however, this will throw an error.
