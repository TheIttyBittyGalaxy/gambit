#ifndef EXPRESSION_H
#define EXPRESSION_H

enum class Precedence
{ // Happens last
    None,
    Term,
    Factor,
    Unary,
    Index,
    Match,
}; // Happens first

#endif