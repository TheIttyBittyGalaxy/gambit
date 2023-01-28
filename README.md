# ♟ Gambit

Gambit is a programming language designed for creating turn-based games with complex rule sets. The compiler generates a playable executable with a built in [Monte-Carlo Tree Search](https://en.wikipedia.org/wiki/Monte_Carlo_tree_search) AI. Gambit's [entity system](/documentation/overview.md#entities) allows game objects to be declared and iteratively extended according to complex rule sets (and avoids the [Deadly Diamond of Death](https://en.wikipedia.org/wiki/Multiple_inheritance#The_diamond_problem)!). It puts an emphasis on functional and declarative programming features to help handle the complexity of games with many rules that interact with one another.

## Development Status

Gambit is still in early development, and isn't usable yet. It's a hobby project I'm working on in my spare time. By all means feel free to contribute code and ideas - [create an issue](/issues/new) to let me know! Questions and feedback are also welcomed!

### Compiler

| Stage                           | Progress       |
| ------------------------------- | -------------- |
| **Parsing**                     | 🟡 In progress |
| **Resolution phase**            | 🟡 In progress |
| **Type and semantics checking** | 🔴 Not started |
| **Code generation**             | 🔴 Not started |
| **MCTS AI generation**          | 🔴 Not started |

### Language Design

You can get a sense of the current state of the language by looking at the [example programs](game), or the _(rough)_ [language documentation](documentation/overview.md).

The core properties of the language (entities, selectors, pure functions, and an emphasis on functional and declarative features) have been established, and are based on my experience trying to develop an AI for the game [Disney© Villainous](https://www.ravensburger.co.uk/en-GB/products/games/experience/villainous). More work is needed to figure out the most helpful set of functional and declarative features. The largest unsolved challenge is how Gambit should handle [hidden state](https://en.wikipedia.org/wiki/Perfect_information). I plan to finalize the syntax only after the semantics of the language have been figured out, to avoid reworking it again later.

## Repository Contents

-   **[documentation/overview.md](documentation/overview.md)**: How to use the language and it's various features. (Currently, this is less of an accurate, helpful document - and more of a place for me to keep notes!)
-   **[game](game)**: Popular games written in Gambit.
-   **[compiler](compiler)**: The Gambit compiler written in C++. The built executable is _(will be)_ able to compile Gambit projects into a playable executable.
-   **[editor/vscode](editor/vscode)**: A Visual Studio Code extension for the Gambit Language
