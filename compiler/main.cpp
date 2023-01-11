#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <variant>
#include <vector>
using namespace std;

// POINTERS //

template <class T>
using ptr = shared_ptr<T>;

template <class T>
using wptr = weak_ptr<T>;

// TOKENS //

struct Token
{
    enum Kind
    {
        Line,

        Equal,
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
        ParenL,
        ParenR,
        CurlyL,
        CurlyR,
        SquareL,
        SquareR,
        TrigL,
        TrigR,

        KeyEnum,
        KeyExtend,
        KeyConstruct,
        KeyStatic,
        KeyState,
        KeyProperty,
        KeyBreak,
        KeyContinue,
        KeyElse,
        KeyFor,
        KeyIf,
        KeyIn,
        KeyMatch,
        KeyReturn,
        KeyUntil,
        KeyChoose,
        KeyFilter,
        KeyTransform,
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

    Token(Kind kind, string str, size_t line, size_t column) : kind(kind),
                                                               str(str),
                                                               line(line),
                                                               column(column) {}
};

const map<Token::Kind, string> token_name = {
    {Token::Line, "Line"},

    {Token::Equal, "Equal"},
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
    {Token::ParenL, "ParenL"},
    {Token::ParenR, "ParenR"},
    {Token::CurlyL, "CurlyL"},
    {Token::CurlyR, "CurlyR"},
    {Token::SquareL, "SquareL"},
    {Token::SquareR, "SquareR"},
    {Token::TrigL, "TrigL"},
    {Token::TrigR, "TrigR"},

    {Token::KeyEnum, "KeyEnum"},
    {Token::KeyExtend, "KeyExtend"},
    {Token::KeyConstruct, "KeyConstruct"},
    {Token::KeyStatic, "KeyStatic"},
    {Token::KeyState, "KeyState"},
    {Token::KeyProperty, "KeyProperty"},
    {Token::KeyBreak, "KeyBreak"},
    {Token::KeyContinue, "KeyContinue"},
    {Token::KeyElse, "KeyElse"},
    {Token::KeyFor, "KeyFor"},
    {Token::KeyIf, "KeyIf"},
    {Token::KeyIn, "KeyIn"},
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
    {Token::LessThanEqual, regex("<=")},
    {Token::GreaterThanEqual, regex(">=")},

    {Token::Add, regex("\\+")},
    {Token::Sub, regex("\\-")},
    {Token::Mul, regex("\\*")},
    {Token::Div, regex("\\\\")},
    {Token::Dot, regex("\\.")},
    {Token::Comma, regex("\\,")},
    {Token::Colon, regex("\\:")},
    {Token::Question, regex("\\?")},
    {Token::Assign, regex("\\=")},
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
    {"enum", Token::KeyEnum},
    {"extend", Token::KeyExtend},
    {"construct", Token::KeyConstruct},

    {"static", Token::KeyStatic},
    {"state", Token::KeyState},
    {"property", Token::KeyProperty},

    {"break", Token::KeyBreak},
    {"continue", Token::KeyContinue},
    {"else", Token::KeyElse},
    {"for", Token::KeyFor},
    {"if", Token::KeyIf},
    {"in", Token::KeyIn},
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

string to_string(Token t)
{
    if (t.kind == Token::Line)
    {
        return "[/]";
    }
    return "[" + token_name.at(t.kind) + " " + t.str + "]";
}

// ERRORS //

vector<string> errors;

void emit_error(string msg)
{
    errors.emplace_back("[Error] " + msg);
}

void emit_error(string msg, size_t line, size_t column)
{
    errors.emplace_back("[Error at " + to_string(line) + ":" + to_string(column) + "] " + msg);
}

void emit_error(string msg, Token t)
{
    emit_error(msg, t.line, t.column);
}

// LEXING //

vector<Token> generate_tokens(string src)
{
    vector<Token> tokens;
    size_t line = 1;
    size_t column = 1;
    size_t position = 0;
    string sub = src;

    auto advance = [&](int amt)
    {
        column += amt;
        position += amt;
        sub = src.substr(position);
    };

    auto advance_line = [&]()
    {
        line += 1;
        column = 1;
        position += 1;
        sub = src.substr(position);
    };

    size_t multi_line_comment_nesting = 0;
    bool is_line_comment = false;
    bool panic_mode = false;

    while (position < src.length())
    {
        bool error_occurred = false;
        string next = sub.substr(0, 1);
        string next_two = sub.substr(0, 2);

        if (is_line_comment)
        {
            if (next == "\n")
            {
                tokens.emplace_back(Token::Line, "\n", line, column);
                advance_line();
                is_line_comment = false;
            }
            else
            {
                advance(1);
            }
        }

        else if (next_two == "/*")
        {
            multi_line_comment_nesting++;
            advance(2);
        }

        else if (multi_line_comment_nesting > 0)
        {
            if (next_two == "*/")
            {
                multi_line_comment_nesting--;
                advance(2);
            }
            else if (next == "\n")
            {
                advance_line();
            }
            else
            {
                advance(1);
            }
        }

        else if (next_two == "//")
        {
            is_line_comment = true;
            advance(2);
        }

        else if (next == "\n")
        {
            tokens.emplace_back(Token::Line, "\n", line, column);
            advance_line();
        }

        else if (next == " " || next == "\t")
        {
            advance(1);
        }

        else
        {
            bool character_parsed = false;
            for (auto rule : token_match_rules)
            {
                smatch info;
                if (regex_search(sub, info, rule.second, regex_constants::match_continuous))
                {
                    Token::Kind kind = rule.first;
                    string str = info.str();

                    if (kind == Token::Identity)
                    {
                        for (auto key_rule : keyword_match_rules)
                        {
                            if (str == key_rule.first)
                            {
                                kind = key_rule.second;
                                break;
                            }
                        }
                    }

                    tokens.emplace_back(kind, str, line, column);
                    advance(info.length());
                    character_parsed = true;
                    break;
                }
            }

            if (!character_parsed)
            {
                if (!panic_mode)
                    emit_error("Unrecognised character " + next, line, column); // FIXME: Make this a proper error message.}
                advance(1);
                panic_mode = true;
                error_occurred = true;
            }
        }

        panic_mode = error_occurred;
    }

    return tokens;
}

// PROGRAM MODEL //

struct Program;
struct Scope;

struct NativeType;
struct Construct;
struct ConstructField;
using Entity = std::variant<ptr<NativeType>, ptr<Construct>>;

struct Literal;
using Expression = std::variant<ptr<Literal>>;

// Macros

#define IS(variant_value, T) (holds_alternative<T>(variant_value))
#define AS(variant_value, T) (get<T>(variant_value))

#define IS_PTR(variant_value, T) (holds_alternative<ptr<T>>(variant_value))
#define AS_PTR(variant_value, T) (get<ptr<T>>(variant_value))

// Program

struct Program
{
    ptr<Scope> global_scope;
};

struct Scope
{
    wptr<Scope> parent;
    map<string, Entity> lookup;
};

// Entities

struct NativeType
{
    string identity;
    string cpp_identity;
};

struct Construct
{
    string identity;
    map<string, ptr<ConstructField>> fields;
};

struct ConstructField
{
    string identity;
    string type;
    bool is_static = false;
    bool is_property = false;
};

// Expressions

struct Literal
{
    variant<double, int, bool, string> value;
};

// SERIALISE PROGRAM MODEL //

// FIXME: Implement versions of these to_json functions that instead of outputing `string`,
//        output a `json` data structure, and then implement `to_string(json)`. This way
//        `to_string(json)` can handle pretty printing a JSON output, and the `to_json`
//        functions can just crudely convert to the correct data structure.

string to_json(ptr<Program> program);
string to_json(ptr<Scope> scope);
string to_json(ptr<NativeType> native_type);
string to_json(ptr<Construct> construct);
string to_json(ptr<ConstructField> field);
string to_json(Entity entity);
string to_json(ptr<Literal> literal);
string to_json(Expression expression);

string to_json(ptr<Program> program)
{
    string json = "{\"node\": \"Program\", ";
    json += "\"global_scope\": " + to_json(program->global_scope);
    json += "}";
    return json;
}

string to_json(ptr<Scope> scope)
{
    string json = "{\"node\": \"Scope\", ";

    json += "\"lookup\": {";
    for (auto entry : scope->lookup)
        json += "\"" + entry.first + "\": " + to_json(entry.second) + ", ";
    json += "}";

    json += "}";
    return json;
}

string to_json(ptr<NativeType> native_type)
{
    string json = "{\"node\": \"NativeType\", ";
    json += "\"identity\": \"" + native_type->identity + "\", ";
    json += "\"cpp_identity\": \"" + native_type->cpp_identity + "\", ";
    json += "}";
    return json;
}

string to_json(ptr<Construct> construct)
{
    string json = "{\"node\": \"Construct\", ";
    json += "\"identity\": \"" + construct->identity + "\", ";

    json += "\"fields\": {";
    for (auto entry : construct->fields)
        json += "\"" + entry.first + "\": " + to_json(entry.second) + ", ";
    json += "}";

    json += "}";
    return json;
}

string to_json(ptr<ConstructField> field)
{
    string json = "{\"node\": \"ConstructField\", ";
    json += "\"identity\": \"" + field->identity + "\", ";
    json += "\"type\": \"" + field->type + "\", ";
    json += "\"is_static\": " + (string)(field->is_static ? "true, " : "false, ");
    json += "\"is_property\": " + (string)(field->is_property ? "true" : "false");
    json += "}";
    return json;
}

string to_json(Entity entity)
{
    if (IS_PTR(entity, NativeType))
    {
        return to_json(AS_PTR(entity, NativeType));
    }
    else if (IS_PTR(entity, Construct))
    {
        return to_json(AS_PTR(entity, Construct));
    }
    throw "Unable to serialise Entity node";
}

string to_json(ptr<Literal> literal)
{
    string json = "{\"node\": \"Literal\", \"value\": ";
    if (holds_alternative<double>(literal->value))
        json += to_string(get<double>(literal->value));
    else if (holds_alternative<int>(literal->value))
        json += to_string(get<int>(literal->value));
    else if (holds_alternative<bool>(literal->value))
        json += get<bool>(literal->value) ? "true" : "false";
    else if (holds_alternative<string>(literal->value))
        json += "\"" + get<string>(literal->value) + "\""; // FIXME: Does not properly escape the string
    json += "}";
    return json;
}

string to_json(Expression expression)
{
    if (IS_PTR(expression, Literal))
    {
        return to_json(AS_PTR(expression, Literal));
    }
    throw "Unable to serialise Expression node";
}

// PARSER //

class Parser
{
public:
    ptr<Program> parse(vector<Token> new_tokens)
    {
        tokens = new_tokens;
        current_token = 0;
        panic_mode = false;

        parse_program();
        return program;
    };

private:
    vector<Token> tokens;
    ptr<Program> program = nullptr;
    size_t current_token;
    bool panic_mode;

    class Error : public std::exception
    {
    public:
        string msg;
        Token token;

        Error(string msg, Token token) : msg(msg), token(token){};

        string what()
        {
            return msg;
        }
    };

    bool end_of_file()
    {
        return current_token >= tokens.size();
    }

    bool peek(Token::Kind kind)
    {
        if (end_of_file())
            return false;

        if (kind == Token::Line)
            return tokens.at(current_token).kind == kind;

        size_t i = current_token;
        while (tokens.at(i).kind == Token::Line)
            i++;
        return tokens.at(i).kind == kind;
    };

    Token eat(Token::Kind kind)
    {
        if (end_of_file())
        {
            Token token = tokens.at(tokens.size() - 1);
            throw Error("Expected " + token_name.at(kind) + ", got end of file", token);
        }

        Token token = tokens.at(current_token);

        if (!peek(kind))
        {
            Token::Kind other = token.kind;
            throw Error("Expected " + token_name.at(kind) + ", got " + token_name.at(other), token);
        }

        if (kind != Token::Line)
        {
            while (token.kind == Token::Line)
            {
                current_token++;
                token = tokens.at(current_token);
            }
        }

        current_token++;
        return token;
    };

    bool match(Token::Kind kind)
    {
        if (peek(kind))
        {
            eat(kind);
            return true;
        }
        return false;
    };

    void parse_program()
    {
        program = ptr<Program>(new Program);
        program->global_scope = ptr<Scope>(new Scope);

        while (current_token < tokens.size())
        {
            try
            {
                while (match(Token::Line))
                    if (end_of_file())
                        break;

                if (peek_construct_definition())
                {
                    panic_mode = false;
                    parse_construct_definition(program->global_scope);
                }
                else
                {
                    throw Error("Expected Construct definition", tokens.at(current_token));
                }
            }
            catch (Error err)
            {
                if (!panic_mode)
                    emit_error(err.msg, err.token);
                panic_mode = true;

                while (!match(Token::Line))
                    eat(tokens.at(current_token).kind);
            }
        }
    };

    bool peek_construct_definition()
    {
        return peek(Token::KeyExtend);
    }

    void parse_construct_definition(ptr<Scope> scope)
    {
        auto construct = ptr<Construct>(new Construct);

        eat(Token::KeyExtend);
        construct->identity = eat(Token::Identity).str;

        // FIXME: Use some form of "declare" function, rather than adding to the map directly
        scope->lookup.insert({construct->identity, construct});

        eat(Token::CurlyL);
        while (peek_construct_field())
        {
            parse_construct_field(scope, construct);
        }
        eat(Token::CurlyR);
    };

    bool peek_construct_field()
    {
        return peek(Token::KeyStatic) || peek(Token::KeyState) || peek(Token::KeyProperty);
    }

    void parse_construct_field(ptr<Scope> scope, ptr<Construct> construct)
    {
        auto field = ptr<ConstructField>(new ConstructField);
        if (match(Token::KeyStatic))
            field->is_static = true;
        else if (match(Token::KeyProperty))
            field->is_property = true;
        else
            eat(Token::KeyState);

        field->type = eat(Token::Identity).str;
        field->identity = eat(Token::Identity).str;

        construct->fields.insert({field->identity, field});
    }

    bool peek_expression()
    {
        return peek_literal();
    }

    Expression parse_expression()
    {
        if (peek_literal())
        {
            return parse_literal();
        }

        throw Error("Expected expression", tokens.at(current_token));
    }

    bool peek_literal()
    {
        return peek(Token::Number) || peek(Token::String) || peek(Token::Boolean);
    }

    ptr<Literal> parse_literal()
    {
        auto literal = ptr<Literal>(new Literal);
        if (peek(Token::Number))
        {
            // FIXME: Treat num and int literals differently
            Token t = eat(Token::Number);
            literal->value = stod(t.str);
        }
        else if (peek(Token::String))
        {
            Token t = eat(Token::String);
            literal->value = t.str;
        }
        else if (peek(Token::Boolean))
        {
            Token t = eat(Token::Boolean);
            literal->value = t.str == "true";
        }
        else
        {
            throw Error("Expected literal", tokens.at(current_token));
        }

        return literal;
    }
};

// MAIN //

int main(int argc, char *argv[])
{
    // FIXME: Allow the path to the source project to be passed directly, rather than infered.
    //        I've just made it easier this way for now to make testing easier for myself.

    // FIXME: Allow for generation of multiple source files.

    string game = (argc == 2) ? (string)argv[1] : "tic-tac-toe";
    string src_path = "game/" + game + "/main.gambit";

    // File loading

    ifstream src_file;
    src_file.open(src_path, ios::in);
    if (!src_file)
    {
        cout << "Source file " << src_path << " could not be loaded" << endl;
        return 0;
    }

    string src((istreambuf_iterator<char>(src_file)), istreambuf_iterator<char>());
    src_file.close();

    // Compile

    cout << "\nLEXING" << endl;
    auto tokens = generate_tokens(src);

    // for (auto t : tokens)
    //     cout << to_string(t) << endl;
    // cout << endl;

    // for (auto t : tokens)
    //     cout << t.str << " ";
    // cout << endl;

    cout << "\nPARSING" << endl;
    Parser parser;
    auto program = parser.parse(tokens);

    std::ofstream parser_output;
    parser_output.open("local/parser_output.json");
    parser_output << to_json(program);
    cout << "Saved parser output to local/parser_output.json" << endl;

    cout << "\nERRORS" << endl;
    for (auto err : errors)
        cout << err << endl;
    cout << endl;

    return 0;
}