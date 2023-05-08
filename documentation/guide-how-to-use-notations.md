# Guide: How to use notations

> // FIXME: This 'guide' was actually an attempt at writing a general documentation for notations. I figured it was worth pulling out and turning into a guide, but it needs some work!

In Gambit, it is possible to define [notations](notation.md) for certain types of value. In essence, they are a way to define custom literals for specific types of values, usually a [structure](structure.md), and are particularly useful in cases where it is inconvenient or impractical to write out an entire structure literal by hand.

Notations are recognised using regular expressions and parsed using functions.

## Creating a notation

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

## Using the notation

Once you have defined a notation, you can use it in your code by enclosing the notation in single quotes. For example, to create a `ManaCost` object with two red and one green mana, you can use the notation `'RRG'`:

## Handling ambiguity

In cases where a notated value matches multiple possible notations, Gambit will usually be able to infer the correct type from context. However, if it cannot infer a single type, an error will be thrown.
