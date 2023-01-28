#include "apm.h"
#include "json.h"
#include <exception>

// Macros

#define STRUCT_PTR_FIELD(field) json.add(#field, node->field);

#define STRUCT_FIELD(field) json.add(#field, node.field);

#define VARIANT(T)   \
    if (IS(node, T)) \
        return to_json(AS(node, T), depth);

#define VARIANT_PTR(T)   \
    if (IS_PTR(node, T)) \
        return to_json(AS_PTR(node, T), depth);

// Serialisation

string to_json(const ptr<Program> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Program"));
    STRUCT_PTR_FIELD(global_scope);
    json.close();
    return (string)json;
}

string to_json(const Scope::LookupValue &node, const size_t &depth)
{
    VARIANT_PTR(NativeType);
    VARIANT_PTR(EnumType);
    VARIANT_PTR(Entity);

    throw runtime_error("Could not serialise Scope::LookupValue"); // FIXME: Use a proper exception type
}

string to_json(const ptr<Scope> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Scope"));
    STRUCT_PTR_FIELD(lookup);
    json.close();
    return (string)json;
}

string to_json(const ptr<UnresolvedIdentity> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("UnresolvedIdentity"));
    STRUCT_PTR_FIELD(identity);
    json.close();
    return (string)json;
}

string to_json(const ptr<EnumType> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("EnumType"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(values);
    json.close();
    return (string)json;
}

string to_json(const ptr<EnumValue> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("EnumValue"));
    STRUCT_PTR_FIELD(identity);
    json.close();
    return (string)json;
}

string to_json(const ptr<Entity> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Entity"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(fields);
    STRUCT_PTR_FIELD(signature);
    STRUCT_PTR_FIELD(base_definition_found);
    json.close();
    return (string)json;
}

string to_json(const ptr<EntityField> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("EntityField"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(type);
    STRUCT_PTR_FIELD(is_static);
    STRUCT_PTR_FIELD(is_property);
    STRUCT_PTR_FIELD(initializer);
    json.close();
    return (string)json;
}

string to_json(const ptr<NativeType> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("NativeType"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(cpp_identity);
    json.close();
    return (string)json;
}

string to_json(const ptr<OptionalType> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("OptionalType"));
    STRUCT_PTR_FIELD(type);
    json.close();
    return (string)json;
}

string to_json(const ptr<InvalidType> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InvalidType"));
    json.close();
    return (string)json;
}

string to_json(const Type &node, const size_t &depth)
{
    VARIANT_PTR(NativeType);
    VARIANT_PTR(OptionalType);
    VARIANT_PTR(InvalidType);
    VARIANT_PTR(UnresolvedIdentity);
    VARIANT_PTR(EnumType);
    VARIANT_PTR(Entity);

    throw runtime_error("Could not serialise Type node"); // FIXME: Use a proper exception type
}

string to_json(const ptr<Literal> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Literal"));
    if (IS(node->value, double))
        json.add("value", AS(node->value, double));
    else if (IS(node->value, int))
        json.add("value", AS(node->value, int));
    else if (IS(node->value, bool))
        json.add("value", AS(node->value, bool));
    else if (IS(node->value, string))
        json.add("value", AS(node->value, string));
    json.close();
    return (string)json;
}

string to_json(const ptr<ListValue> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("ListValue"));
    STRUCT_PTR_FIELD(values);
    json.close();
    return (string)json;
}

string to_json(const ptr<Unary> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Unary"));
    STRUCT_PTR_FIELD(op);
    STRUCT_PTR_FIELD(value);
    json.close();
    return (string)json;
}

string to_json(const ptr<Binary> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Binary"));
    STRUCT_PTR_FIELD(op);
    STRUCT_PTR_FIELD(lhs);
    STRUCT_PTR_FIELD(rhs);
    json.close();
    return (string)json;
}

string to_json(const Match::Rule &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    STRUCT_FIELD(pattern);
    STRUCT_FIELD(result);
    json.close();
    return (string)json;
}

string to_json(const ptr<Match> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Match"));
    STRUCT_PTR_FIELD(subject);
    STRUCT_PTR_FIELD(rules);
    json.close();
    return (string)json;
}

string to_json(const ptr<InvalidValue> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InvalidValue"));
    json.close();
    return (string)json;
}

string to_json(const Expression &node, const size_t &depth)
{
    VARIANT_PTR(Literal);
    VARIANT_PTR(ListValue);
    VARIANT_PTR(UnresolvedIdentity);
    VARIANT_PTR(EnumValue);
    VARIANT_PTR(Unary);
    VARIANT_PTR(Binary);
    VARIANT_PTR(Match);
    VARIANT_PTR(InvalidValue);

    throw runtime_error("Could not serialise Expression node"); // FIXME: Use a proper exception type
};
