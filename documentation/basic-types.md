# Basic types

> ⚙️ **Development Status:** Partially implemented in the compiler, but much work is needed.

> 🔎 **Implementation Detail:** Internally, the compiler refers to the basic types as 'intrinsic types'.

> // TODO: Complete this documentation

| Type   | Description                                                                                                  |
| ------ | ------------------------------------------------------------------------------------------------------------ |
| `num`  | A number, represented as a [floating point number](https://en.wikipedia.org/wiki/Floating-point_arithmetic). |
| `int`  | An integer number value. Is a sub-type of `num`.                                                             |
| `amt`  | An integer number greater than or equal to 0. Is a sub-type of `int`.                                        |
| `bool` | `true` or `false`.                                                                                           |
| `str`  | A string.                                                                                                    |

## See more

-   [Types](type.md)
-   [Enums](enum.md)
-   [Structures](structure.md)
-   [Entities](entity.md)
-   [Patterns](pattern.md)
