#ifndef Expression_H
#define Expression_H

enum class Precedence
{ // Happens last
    None,
    Term,
    Factor,
    Unary,
    Match,
}; // Happens first

#endif