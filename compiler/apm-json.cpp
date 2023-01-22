#include "apm.h"
#include "json.h"

#define STRUCT_FIELD(field) json.add(#field, node->field);

#define VARIANT(T)   \
    if (IS(node, T)) \
        return to_json(AS(node, T), depth);

#define VARIANT_PTR(T)   \
    if (IS_PTR(node, T)) \
        return to_json(AS_PTR(node, T), depth);

string to_json(const ptr<Program> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Program"));
    STRUCT_FIELD(global_scope);
    json.close();
    return (string)json;
};

string to_json(const Scope::LookupValue &node, const size_t &depth)
{
    VARIANT_PTR(NativeType);
    VARIANT_PTR(EnumType);
    VARIANT_PTR(Entity);

    throw "Could not serialise Scope::LookupValue"; // FIXME: Use a proper exception type
}

string to_json(const ptr<Scope> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Scope"));
    STRUCT_FIELD(lookup);
    json.close();
    return (string)json;
};

string to_json(const ptr<UnresolvedIdentity> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("UnresolvedIdentity"));
    STRUCT_FIELD(identity);
    json.close();
    return (string)json;
};

string to_json(const ptr<NativeType> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("NativeType"));
    STRUCT_FIELD(identity);
    STRUCT_FIELD(cpp_identity);
    json.close();
    return (string)json;
};

string to_json(const ptr<EnumType> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("EnumType"));
    STRUCT_FIELD(identity);
    STRUCT_FIELD(values);
    json.close();
    return (string)json;
};

string to_json(const ptr<EnumValue> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("EnumValue"));
    STRUCT_FIELD(identity);
    json.close();
    return (string)json;
};

string to_json(const ptr<Entity> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Entity"));
    STRUCT_FIELD(identity);
    STRUCT_FIELD(fields);
    STRUCT_FIELD(base_definition_found);
    json.close();
    return (string)json;
};

string to_json(const ptr<EntityField> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("EntityField"));
    STRUCT_FIELD(identity);
    STRUCT_FIELD(type);
    STRUCT_FIELD(is_static);
    STRUCT_FIELD(is_property);
    STRUCT_FIELD(default_value);
    json.close();
    return (string)json;
};

string to_json(const Type &node, const size_t &depth)
{
    VARIANT_PTR(UnresolvedIdentity);
    VARIANT_PTR(NativeType);
    VARIANT_PTR(EnumType);
    VARIANT_PTR(Entity);

    throw "Could not serialise Type node"; // FIXME: Use a proper exception type
};

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
};

string to_json(const Expression &node, const size_t &depth)
{
    VARIANT_PTR(Literal);
    VARIANT_PTR(EnumValue);

    throw "Could not serialise Expression node"; // FIXME: Use a proper exception type
};