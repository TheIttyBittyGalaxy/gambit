> ⚠️ These are rough 'back of the envolope' notes.  
> TODO: Formalise and write up these notes clearly and concisely.

# Constructs, defining, declaring, and instances

A construct is 'a thing that could exist'. It's how we tell Gambit what exists in our game. For example, if your game is played with a regular deck of cards, a `Card` is a construct in your game. Constructs are defined, declared, and instantiated.

-   **Definition**: The shape of the thing (e.g. a card has a suit and a face)
-   **Declaration**: The details for a specific kind of that thing (e.g. the ace of spades is a card with suit 'spades' and face 'ace')
-   **Instance**: The thing itself (e.g. the ace of spades)

If your game were played with two decks of cards, there would be 1 card definition, 52 card declarations, and 104 card instances.

Let's set we wanted to define a regular set of playing cards. We could do that something like this.

```
DEFINE Number_Face { 2, 3, 4, 5, 6, 7, 8, 9 }
DEFINE Picture_Face { ACE, JACK, QUEEN, KING }
DEFINE Face Number_Face | Picture_Face
DEFINE Suit { HEART, DIAMOND, CLUB, SPADE }

DEFINE Card {
	static Suit suit
	static Face face
}
```

This declares what cards _are_, but doesn't actually tell us which can exist. To do that, we must declare each specific card with the appropriate fields. For example, the ace of spades would look like this.

```
DECLARE Card ace_of_spades {
	suit = SPADE
	face = ACE
}
```

In the case of a deck of cards, to do these one by one would be tedious, and so we can use static loops instead.

```
FOR suit IN Suit {
	FOR face IN Face {
		DECLARE Card {
			suit = suit
			face = face
		}
	}
}
```

In this case each card would not have an identifier (e.g. `ace_of_spades`) but could still be accessed with an appropriate selector.

```
Card(suit=SPADE, face=ACE) // Select the ace of spades
Card(suit=HEART, face=3) // Select the three of hearts
Card(suit=CLUB) // Select all clubs
Card(face=ACE) // Select all aces
```

Naturally, this can be a bit cumbersome, and so when we DEINE a card there is a syntax that allows us to define a 'signature' for the construct. This allows us to not have to name the individual fields.

```
DEFINE Card(suit, face) {
	static Suit suit
	static Face face
}

Card(SPADE, ACE) // Select the ace of spades
Card(HEART, 3) // Select the three of hearts
Card(CLUB) // Select all clubs
Card(ACE) // Select all aces
```

Note that in this case, if you only provide a single value to the signature, it is totally unambigous as if to the `suit` or the `face` has been provided, because those two fields have no overlap in their domain of values. If they did, the syntax would be invalid.

The syntax can be shortened even further in by ommitting the type, but only in situations where the type can be infered. e.g.

```
Card card = ...
IF card IS (SPADE, ACE) DO
	...
END
```

## Addative and scoped definitions

Definitions in gambit are 'additive', this means that different fields of a construct can be defined in different places, including across different files. The following is perfectly valid.

```
DEFINE Card {
	static Suit suit
	static Face face
}

DEFINE Card {
	state int tokens
}
```

Additions are also scoped. This means that different scopes can specify _differnt_ fields of a cosntruct with the same name.

```
SCOPE sapphire_expansion {
	DEFINE Card {
		state int tokens // Can only be accessed in the 'sapphire_expansion' scope
	}
}

SCOPE ruby_expansion {
	DEFINE Card {
		state int tokens // Can only be access in the 'ruby_expansion' scope
	}
}
```

# Notation

Often, it can be useful to specify a notion for certian kinds of values that may appear in your game. For example, in Magic the Gathering, cards have a 'mana cost'. There are 5 different 'colours' of mana, and in even the most basic cases, a card may specifiy a cost of any amount of each specific kind of mana, and/or a certian amount of mana of any colour. e.g. the card 'Abundance' require you may 2 green mana and then also 2 mana of any colour. A notation such as `GG2` would be more convient that having to type out all the names and fields of a struct by hand, and while reamainging more readable than a nameless constructor.

Gambit allows you to define a notations for certian kinds of values. You provide Gambit with a regular expression and a function, and then whenever Gambit matches a `%` followed by a match to your regular expression in an expression, it will run your function with the matched notation as it's parameter.

To parse our simpile MTG mana cost notation, we could do something like this

```
STRUCTURE Mana_Cost {
	int red = 0
	int black = 0
	int green = 0
	int blue = 0
	int white = 0
	int colourless = 0
}

NOTATION Mana_Cost ([RBGUW]*)(%d*) -> match, colour, colourless {
	value.colourless = as_integer(colourless)
	FOR c IN colour {
		MATCH C {
			"R": value.red++
			"B": value.black++
			"G": value.green++
			"U": value.blue++
			"W": value.white++
		}
	}
}

...

PROCEDURE do_thing(Mana_Cost cost) {
	 return cost == %RRG
}
```

## Ambigutity

When a notated value matches multiple possible notations, usually this ambigutity is resolved using type inference. e.g. maybe `%2` could annotate a mama cost, or it could annotate a power and thoughness. Usually, Gambit can tell from context. In cases where it can't however, this will throw an error.

# Construct fields

A field can be one of the following

-   **static**: This field may vary from instance to instance, but never changes. (e.g. the suit of a card)
-   **state**: This field can change during the game. (e.g. how many coin tokens the player has)
-   **property**: This field is a value that is determined by assessing the game state. (e.g. in Disney's Villainous, wheter or not a specific action pad is covered)

The distinction between state and property is subtle, but the most concrete way to understand the difference is to know that state is stored in memory, and properties are calculated using other fields.

# Types

The most basic primitive types are `Boolean`, `Integer`, `Number`, and `String`. All constructs are also types. `Procedure` is also a type.

## Optional types

Every type has a corresponding 'optional type', which can be a value of that type or `none`. It's syntax is `Type?`.

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

Values of type `Type!` must be of at least one value in that domain. e.g., with the card definition above, there could be no 'Ace of nothing'. However, all mulitypes also have a corresponding optional multitype, denoted as `Type!?`. Values of an optional multitype may be of zero, one, or multiple values.

To create a multitype value, you can combine multiple values with the `&` operator, e.g. `Spade&Heart`. A distiction between the `==` and `IS` operators is that the `==` seeks to make an exact match between multitype values, and the `IS` operator only checks that every value on the righthand side is also on the lefthand side. e.g.

```
Heart == Heart // true
Heart IS Heart // true

Heart&Spade == Spade // false
Heart&Spade IS Spade // true

Spade == Heart&Spade // false
Spade IS Heart&Spade // false

Heart&Spade == Club&Spade // false
Heart&Spade IS Club&Spade // false
```

## Union types

Multiple types can be combined into one by creating their union. The original types will be considered subtypes of the union. The syntax is `TypeA | Type B`.

> ! How should this interact with multi-types?? Presumably something like `A!|B` is valid, and means you can be either one or more of A, or just one of B? So `A!|B` is not equal to `(A|B)!`. Though, confusingly, `A!?|B` is equal to `(A!|B)?`

# Functions & Procedures

Functions and procedures in Gambit are two different things. The only difference between them is that functions must be 'mathematically pure', or in other words, cannot change the game state. Functions are 'read-only' procedures.

Properties are determined using functions.

> ???? maybe??? can it all be done with selectors?? do you even need the notion of a 'property' when the programer could just use functions directly?? I guess properties as a seperate construct set things up nicely to be easily cachable? is that even useful?

## First class functions and procedures

Functions and procedures can be first class. This also means they can be the fields of constructs. For example, say that some cards in your game have an 'activate' ability, that could be implemented something like this.

```
DEFINE Card {
	static Procedure(Player user)? activate_ability
	state bool activated = false
	...
}

PROCEDURE activate_card(Player player, Card card) {
	IF card HAS activate_ability AND NOT card.activated {
		card.activate_ability(player)
		card.activated = true
	}
}

PROCEDURE gain_two_money(Player player) {
	player.money += 2
}

DECLARE Card atm {
	activate_ability = gain_two_money
}

DECLARE Card jackpot {
	PROCEDURE activate_ability(user) {
		card.user.money += roll(6) + roll(6)
	}
}
```

# Selectors

The values of construct fields can be set when they are declared. However, you can also set fields using construct selectors. For example, lets say that in your game, cards can accumulate tokens. A card becomes 'activated' either when it has at least 3 tokens, or just 1 token if it happens to be an ace. That could be implemented something like this.

```
DEFINE Card {
	static { HEART, DIAMOND, CLUB, SPADE } suit
	static { ACE, 2, 3, 4, 5, 6, 7, 8, 9, JACK, QUEEN, KING } face
	state int tokens = 0
	property bool activated = false
	/*
	This is the equivelent of
	SELECT Card {
		activated = false
	}
	*/
}

SELECT Card(tokens>=3) {
	activated = true
}

SELECT Card(face=ACE, tokens>=1) {
	activated = true
}
```

Similar to other declarative languages such as CSS. More specific selectors override less specific ones. So the `Card(face=ACE, tokens>=1)` selector overrides the `Card(tokens>=3)` selector, which in turn overrides the base case specified in the `Card` definition.

> ? What should happen in the situation where there are multiple selectors that are both a) equal in specificity and b) have overlap in the values they can select? In CSS, the selector further down in the document takes precedence, but I don't think that is sufficient for our purposes, particually as the programmer has no control over the order in which files load.

As well as being set to direct values, properties in selectors can be set to pure expressions (expressions that do not affect the game state). Therefore, you could rewrite the code above as the following.

```
DEFINE Card {
	static { HEART, DIAMOND, CLUB, SPADE } suit
	static { ACE, 2, 3, 4, 5, 6, 7, 8, 9, JACK, QUEEN, KING } face
	state int tokens = 0
	property bool activated = tokens >= 3 OR (tokens >= 1 AND face == ACE)
}
```

The selector system is most advantageous in situations where different instances of a construct may behave in very specific and different ways. e.g. in Disney's villainous, a card's kind is _almost_ always a single static value, but in some cases can be changed by the abilities of other cards. e.g. Jafar can 'hypnotise' 'Hero' cards, at which point they become 'Ally' cards. Split across two files, that could look something like this.

**main.gambit**

```
DEFINE Card_Kind { HERO, ALLY, ... }
DEFINE Card {
	property Card_Kind kind
	...
}
```

**jaffar.gambit**

```
DEFINE Card {
	state bool hypnotised = false
}

DECLARE Card aladin {
	kind = HERO
	....
}

SELECT Card(hyponotised = true) {
	kind = ALLY
}
```
