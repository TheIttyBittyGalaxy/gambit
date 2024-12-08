#pragma once
#ifndef EXPRESSION_H
#define EXPRESSION_H

enum class Precedence
{ // Happens last
    None,
    Choose,
    LogicalOr,
    LogicalAnd,
    CompareEqual,
    CompareRelative,
    Term,
    Factor,
    Unary,
    Index,
    Call,
    ExpressionStatement, // e.g. `MatchExpression`, `IfExpression`
}; // Happens first

#endif