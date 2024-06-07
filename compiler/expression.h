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
    ExpressionStatement, // e.g. `Match`, `IfExpression`
}; // Happens first

#endif