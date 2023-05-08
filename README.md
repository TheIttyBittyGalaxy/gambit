# ♟ Gambit

**Gambit is a programming language designed for implementing turn-based games with complex rule sets.**

It uses an approach to creating [entities](/documentation/entity.md) and their [properties](/documentation/property.md) meant for modelling complex game state and the effects of layered rule sets. The language combines functional and imperative features so that different games can be modelled however is most natural for the game in question.

Gambit is still in early development. It's a hobby project I'm working on in my spare time. My ultimate goal is is to have the compiler generate not only a playable executable, but one with a built in [Monte-Carlo Tree Search](https://en.wikipedia.org/wiki/Monte_Carlo_tree_search) AI player! (The project actually started as an attempt to create such an AI for [Disney© Villainous](https://www.ravensburger.co.uk/en-GB/products/games/experience/villainous), but now I'm _many_ tangents in!)

You can get a sense of the current state of the language by looking at the [example programs](game), or the _very rough_ [documentation](documentation). By all means feel free to contribute code and ideas - [create an issue](/issues/new) or pull request! Questions and feedback are also welcomed! If you're at all interested in the project, I'd love to hear from you. :)

## Development Status

| Aspect                              | Status              |
| ----------------------------------- | ------------------- |
| **Language Design**                 | 🟡 In progress      |
| **Language Documentation**          | 🟠 Rough references |
| **Parsing**                         | 🟡 In progress      |
| **Type Checking & Static Analysis** | 🟡 In progress      |
| **Playable Program Generation**     | 🔴 Not started      |
| **MCTS AI Generation**              | 🔴 Not started      |

## Repository Contents

-   **[documentation](documentation)**: References and guides on the language and it's features.
-   **[game](game)**: Example games written in Gambit.
-   **[compiler](compiler)**: The Gambit compiler written in C++.
-   **[test](test)**: Sample programs for testing the compiler.
-   **[editor/vscode](editor/vscode)**: A Visual Studio Code extension for the Language.
