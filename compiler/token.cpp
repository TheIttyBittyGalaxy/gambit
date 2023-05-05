#include "token.h"
using namespace std;

string to_string(Token t)
{
    if (t.kind == Token::InvalidToken)
        return "[INVALID]";
    if (t.kind == Token::Line)
        return "[" + to_string(t.line) + ":" + to_string(t.column) + " /]";
    return "[" + to_string(t.line) + ":" + to_string(t.column) + " " + token_name.at(t.kind) + " " + t.str + "]";
}

const map<Token::Kind, string> token_name = {
    {Token::InvalidToken, "InvalidToken"},

    {Token::Line, "Line"},

    {Token::Equal, "Equal"},
    {Token::NotEqual, "NotEqual"},
    {Token::LessThanEqual, "LessThanEqual"},
    {Token::GreaterThanEqual, "GreaterThanEqual"},

    {Token::Add, "Add"},
    {Token::Sub, "Sub"},
    {Token::Mul, "Mul"},
    {Token::Div, "Div"},
    {Token::Dot, "Dot"},
    {Token::Comma, "Comma"},
    {Token::Colon, "Colon"},
    {Token::Question, "Question"},
    {Token::Assign, "Assign"},
    {Token::Hash, "Hash"},
    {Token::ParenL, "ParenL"},
    {Token::ParenR, "ParenR"},
    {Token::CurlyL, "CurlyL"},
    {Token::CurlyR, "CurlyR"},
    {Token::SquareL, "SquareL"},
    {Token::SquareR, "SquareR"},
    {Token::TrigL, "TrigL"},
    {Token::TrigR, "TrigR"},

    {Token::KeyEntity, "KeyEntity"},
    {Token::KeyEnum, "KeyEnum"},
    {Token::KeyFn, "KeyFn"},
    {Token::KeyStatic, "KeyStatic"},
    {Token::KeyState, "KeyState"},
    {Token::KeyBreak, "KeyBreak"},
    {Token::KeyContinue, "KeyContinue"},
    {Token::KeyElse, "KeyElse"},
    {Token::KeyFor, "KeyFor"},
    {Token::KeyIf, "KeyIf"},
    {Token::KeyIn, "KeyIn"},
    {Token::KeyLoop, "KeyLoop"},
    {Token::KeyMatch, "KeyMatch"},
    {Token::KeyReturn, "KeyReturn"},
    {Token::KeyUntil, "KeyUntil"},
    {Token::KeyChoose, "KeyChoose"},
    {Token::KeyFilter, "KeyFilter"},
    {Token::KeyTransform, "KeyTransform"},
    {Token::KeyAnd, "KeyAnd"},
    {Token::KeyOr, "KeyOr"},
    {Token::KeyNot, "KeyNot"},

    {Token::Boolean, "Boolean"},
    {Token::Number, "Number"},
    {Token::String, "String"},
    {Token::Identity, "Identity"},
};

const map<Token::Kind, regex> token_match_rules = {
    {Token::Line, regex("\n")},

    {Token::Equal, regex("==")},
    {Token::NotEqual, regex("!=")},
    {Token::LessThanEqual, regex("<=")},
    {Token::GreaterThanEqual, regex(">=")},

    {Token::Add, regex("\\+")},
    {Token::Sub, regex("\\-")},
    {Token::Mul, regex("\\*")},
    {Token::Div, regex("\\/")},
    {Token::Dot, regex("\\.")},
    {Token::Comma, regex("\\,")},
    {Token::Colon, regex("\\:")},
    {Token::Question, regex("\\?")},
    {Token::Assign, regex("\\=")},
    {Token::Hash, regex("\\#")},
    {Token::ParenL, regex("\\(")},
    {Token::ParenR, regex("\\)")},
    {Token::CurlyL, regex("\\{")},
    {Token::CurlyR, regex("\\}")},
    {Token::SquareL, regex("\\[")},
    {Token::SquareR, regex("\\]")},
    {Token::TrigL, regex("\\<")},
    {Token::TrigR, regex("\\>")},

    {Token::Number, regex("[0-9]+(\\.[0-9]+)?")},
    {Token::String, regex("\"(\\.|.)*?\"")}, // FIXME: This regular expression matches "\", which is incorrect
    {Token::Identity, regex("[a-zA-Z][a-zA-Z0-9_]*")},
};

const map<string, Token::Kind> keyword_match_rules = {
    {"entity", Token::KeyEntity},
    {"enum", Token::KeyEnum},
    {"fn", Token::KeyFn},

    {"static", Token::KeyStatic},
    {"state", Token::KeyState},

    {"break", Token::KeyBreak},
    {"continue", Token::KeyContinue},
    {"else", Token::KeyElse},
    {"for", Token::KeyFor},
    {"if", Token::KeyIf},
    {"in", Token::KeyIn},
    {"loop", Token::KeyLoop},
    {"match", Token::KeyMatch},
    {"return", Token::KeyReturn},
    {"until", Token::KeyUntil},

    {"choose", Token::KeyChoose},
    {"filter", Token::KeyFilter},
    {"transform", Token::KeyTransform},

    {"and", Token::Kind ::KeyAnd},
    {"or", Token::KeyOr},
    {"not", Token::KeyNot},

    {"true", Token::Boolean},
    {"false", Token::Boolean},
};