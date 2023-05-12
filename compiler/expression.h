#pragma once
#ifndef EXPRESSION_H
#define EXPRESSION_H

enum class Precedence
{ // Happens last
    None,
    LogicalOr,
    LogicalAnd,
    Term,
    Factor,
    Unary,
    Index,
    Match,
}; // Happens first

#endif