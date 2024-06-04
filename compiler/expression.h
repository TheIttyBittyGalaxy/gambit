#pragma once
#ifndef EXPRESSION_H
#define EXPRESSION_H

enum class Precedence
{ // Happens last
    None,
    LogicalOr,
    LogicalAnd,
    CompareEqual,
    CompareRelative,
    Term,
    Factor,
    Unary,
    Index,
    Call,
    Match,
}; // Happens first

#endif