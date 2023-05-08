> // FIXME: Work through these notes, getting rid of things that aren't useful any more, and turning things that are useful into proper documentation.

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
