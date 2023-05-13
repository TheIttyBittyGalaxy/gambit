#pragma once
#ifndef TOKEN_H
#define TOKEN_H

#include "source.h"
#include <map>
#include <regex>
#include <string>
using namespace std;

struct Token
{
    enum Kind
    {
        InvalidToken,

        Line,
        EndOfFile,

        Equal,
        NotEqual,
        LessThanEqual,
        GreaterThanEqual,

        Add,
        Sub,
        Mul,
        Div,
        Dot,
        Comma,
        Colon,
        Question,
        Assign,
        Hash,
        ParenL,
        ParenR,
        CurlyL,
        CurlyR,
        SquareL,
        SquareR,
        TrigL,
        TrigR,

        KeyEntity,
        KeyEnum,
        KeyFn,
        KeyState,
        KeyBreak,
        KeyContinue,
        KeyElse,
        KeyFor,
        KeyIf,
        KeyIn,
        KeyLoop,
        KeyMatch,
        KeyReturn,
        KeyUntil,
        KeyChoose,
        KeyFilter,
        KeyMap,
        KeyAnd,
        KeyOr,
        KeyNot,

        Boolean,
        Number,
        String,
        Identity,
    };

    Kind kind;
    string str;
    size_t line;
    size_t column;

    // FIXME: As it currently stands, every token contains a reference to the source
    //        so that when we convert them into spans later, it's easy for the merge
    //        function to reference the original source code. Storing the same pointer
    //        on every token individually does feel like a waste of space though -
    //        could this be done better? Perhaps the list of tokens could be included
    //        on the source object itself?
    const Source *source;

    Token() : kind(Token::InvalidToken), str(""), line(0), column(0), source(nullptr){};

    Token(Kind kind, string str, size_t line, size_t column, const Source *source) : kind(kind),
                                                                                     str(str),
                                                                                     line(line),
                                                                                     column(column),
                                                                                     source(source) {}
};

string to_string(Token t);

extern const map<Token::Kind, string> token_name;
extern const map<Token::Kind, regex> token_match_rules;
extern const map<string, Token::Kind> keyword_match_rules;

#endif