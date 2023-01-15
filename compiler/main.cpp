#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <variant>
#include <vector>
#include "token.h"
#include "utilty.h"
#include "lexing.h"
#include "errors.h"
#include "apm.h"
#include "parser.h"
using namespace std;

// JSON //

// FIXME: Only _after_ implementing this system did I realise it would probably be more efficient
//        (both in memory and time usage) to implement some form of "JsonSeraliser" that has utilty
//        methods for creating objects and arrays that _directly_ output valid JSON strings. This way
//        you could implement "JsonSeraliser::to_json" overloads for each of your structs, and then
//        create JSON seralisations without having to rebuild your whole data structure!
//
//        Never mind, work for another day!

struct Json;
using JsonArray = vector<Json>;
using JsonObject = map<string, Json>;
struct Json
{
    using Value = variant<string, int, double, bool, monostate, JsonArray, JsonObject>;
    Value value;

    Json() : value(monostate()){};
    Json(string value) : value(value){};
    Json(int value) : value(value){};
    Json(double value) : value(value){};
    Json(bool value) : value(value){};
    Json(monostate value) : value(value){};
    Json(JsonArray value) : value(value){};
    Json(JsonObject value) : value(value){};
};

Json to_json(Json value) { return value; }
Json to_json(string value) { return Json(value); }
Json to_json(int value) { return Json(value); }
Json to_json(double value) { return Json(value); }
Json to_json(bool value) { return Json(value); }
Json to_json(monostate value) { return Json(value); }

template <typename T>
Json to_json(optional<T> value)
{
    return value.has_value() ? to_json(value.value()) : Json();
}

template <typename T>
Json to_json(vector<T> vector)
{
    JsonArray array;
    for (auto elem : vector)
        array.emplace_back(to_json(elem));
    return array;
}

template <typename T>
Json to_json(map<string, T> map)
{
    JsonObject object;
    for (auto entry : map)
        object.emplace(entry.first, to_json(entry.second));
    return object;
}

string json_str(Json json, size_t depth = 1)
{
    auto value = json.value;
    if (IS(value, string))
        return '"' + AS(value, string) + '"'; // FIXME: Does not correctly escape the string

    else if (IS(value, int))
        return to_string(AS(value, int));

    else if (IS(value, double))
        return to_string(AS(value, double));

    else if (IS(value, bool))
        return AS(value, bool) ? "true" : "false";

    else if (IS(value, monostate))
        return "null";

    else if (IS(value, JsonArray))
    {
        auto array = AS(value, JsonArray);
        if (array.size() == 0)
            return "[]";

        string str = "[";
        string sep = "\n" + string(depth, '\t');
        bool first_value = true;

        depth++;
        for (auto elem : array)
        {
            if (first_value)
                first_value = false;
            else
                str += ",";
            str += sep + json_str(elem, depth);
        }

        str += "\n" + string(depth - 2, '\t') + "]";
        return str;
    }

    else if (IS(value, JsonObject))
    {
        auto object = AS(value, JsonObject);
        if (object.size() == 0)
            return "{}";

        string str = "{";
        string sep = "\n" + string(depth, '\t');
        bool first_value = true;

        depth++;
        for (auto entry : object)
        {
            if (first_value)
                first_value = false;
            else
                str += ",";
            str += sep + "\"" + entry.first + "\": " + json_str(entry.second, depth);
        }

        str += "\n" + string(depth - 2, '\t') + "}";
        return str;
    }

    throw "Cannot serialise invalid JSON value.";
}

// SERIALISE PROGRAM MODEL //

// FIXME: Implement versions of these to_json functions that instead of outputing `string`,
//        output a `json` data structure, and then implement `to_string(json)`. This way
//        `to_string(json)` can handle pretty printing a JSON output, and the `to_json`
//        functions can just crudely convert to the correct data structure.

Json to_json(ptr<Program> program);
Json to_json(ptr<Scope> scope);
Json to_json(ptr<NativeType> native_type);
Json to_json(ptr<EnumType> enum_type);
Json to_json(ptr<EnumValue> enum_value);
Json to_json(ptr<Entity> entity);
Json to_json(ptr<EntityField> field);
Json to_json(Scope::LookupValue entity);
Json to_json(ptr<Literal> literal);
Json to_json(Expression expression);

Json to_json(ptr<Program> program)
{
    return JsonObject({
        {"node", string("Program")},
        {"global_scope", to_json(program->global_scope)},
    });
}

Json to_json(ptr<Scope> scope)
{
    return JsonObject({
        {"node", string("Scope")},
        {"lookup", to_json<Scope::LookupValue>(scope->lookup)},
    });
}

Json to_json(ptr<NativeType> native_type)
{
    return JsonObject({
        {"node", string("NativeType")},
        {"identity", native_type->identity},
        {"cpp_identity", native_type->cpp_identity},
    });
}

Json to_json(ptr<EnumType> enum_type)
{
    return JsonObject({
        {"node", string("EnumType")},
        {"identity", enum_type->identity},
        {"values", to_json<ptr<EnumValue>>(enum_type->values)},
    });
}

Json to_json(ptr<EnumValue> enum_value)
{
    return JsonObject({
        {"node", string("EnumValue")},
        {"identity", enum_value->identity},
    });
}

Json to_json(ptr<Entity> entity)
{
    return JsonObject({
        {"node", string("Entity")},
        {"identity", entity->identity},
        {"fields", to_json<ptr<EntityField>>(entity->fields)},
        {"base_definition_found", entity->base_definition_found},
    });
}

Json to_json(ptr<EntityField> field)
{
    return JsonObject({
        {"node", string("EntityField")},
        {"identity", field->identity},
        {"type", field->type},
        {"is_static", field->is_static},
        {"is_property", field->is_property},
        {"default_value", to_json<Expression>(field->default_value)},
    });
}

Json to_json(Scope::LookupValue value)
{
    if (IS_PTR(value, NativeType))
        return to_json(AS_PTR(value, NativeType));
    else if (IS_PTR(value, EnumType))
        return to_json(AS_PTR(value, EnumType));
    else if (IS_PTR(value, Entity))
        return to_json(AS_PTR(value, Entity));

    throw "Unable to serialise Scope::LookupValue";
}

Json to_json(ptr<Literal> literal)
{
    auto value = literal->value;

    if (IS(value, double))
        return JsonObject({
            {"node", string("Literal")},
            {"value", AS(value, double)},
        });

    if (IS(value, int))
        return JsonObject({
            {"node", string("Literal")},
            {"value", AS(value, int)},
        });

    if (IS(value, bool))
        return JsonObject({
            {"node", string("Literal")},
            {"value", AS(value, bool)},
        });

    if (IS(value, string))
        return JsonObject({
            {"node", string("Literal")},
            {"value", AS(value, string)},
        });

    throw "Unable to seralise Literal node";
}

Json to_json(Expression expression)
{
    if (IS_PTR(expression, Literal))
        return to_json(AS_PTR(expression, Literal));

    throw "Unable to serialise Expression node";
}

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
    parser_output << json_str(to_json(program));
    cout << "Saved parser output to local/parser_output.json" << endl;

    cout << "\nERRORS" << endl;
    for (auto err : errors)
        cout << err << endl;
    cout << endl;

    return 0;
}