#include "apm.h"
#include "json.h"

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

string to_json(const ptr<CodeBlock> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("CodeBlock"));
    STRUCT_PTR_FIELD(singleton_block);
    STRUCT_PTR_FIELD(scope);
    STRUCT_PTR_FIELD(statements);
    json.close();
    return (string)json;
}

string to_json(const Scope::LookupValue &node, const size_t &depth)
{
    VARIANT_PTR(Variable);
    VARIANT_PTR(NativeType);
    VARIANT_PTR(EnumType);
    VARIANT_PTR(Entity);
    VARIANT_PTR(StateProperty);
    VARIANT_PTR(FunctionProperty);
    VARIANT_PTR(Scope::OverloadedIdentity);

    throw json_serialisation_error("Could not serialise Scope::LookupValue variant.");
}

string to_json(const ptr<Scope::OverloadedIdentity> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Scope::OverloadedIdentity"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(overloads);
    json.close();
    return (string)json;
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

string to_json(const ptr<Variable> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Variable"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(pattern);
    json.close();
    return (string)json;
}

string to_json(const ptr<OptionalPattern> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("OptionalPattern"));
    STRUCT_PTR_FIELD(pattern);
    json.close();
    return (string)json;
}

string to_json(const ptr<InvalidPattern> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InvalidPattern"));
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
    json.close();
    return (string)json;
}

string to_json(const ptr<StateProperty> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("StateProperty"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(pattern);
    STRUCT_PTR_FIELD(scope);
    STRUCT_PTR_FIELD(parameters);
    STRUCT_PTR_FIELD(initial_value);
    json.close();
    return (string)json;
}

string to_json(const ptr<FunctionProperty> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("FunctionProperty"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(pattern);
    STRUCT_PTR_FIELD(scope);
    STRUCT_PTR_FIELD(parameters);
    STRUCT_PTR_FIELD(body);
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

string to_json(const Pattern &node, const size_t &depth)
{

    VARIANT_PTR(UnresolvedIdentity);
    VARIANT_PTR(OptionalPattern);
    VARIANT_PTR(InvalidPattern);
    VARIANT_PTR(EnumType);
    VARIANT_PTR(Entity);
    VARIANT_PTR(NativeType);

    throw json_serialisation_error("Could not serialise Pattern variant.");
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

string to_json(const ptr<InstanceList> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InstanceList"));
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

string to_json(const ptr<PropertyIndex> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("PropertyIndex"));

    STRUCT_PTR_FIELD(expr);

    if (IS_PTR(node->property, UnresolvedIdentity))
        json.add("property", AS_PTR(node->property, UnresolvedIdentity));
    else if (IS_PTR(node->property, StateProperty))
        json.add("property", AS_PTR(node->property, StateProperty));
    else if (IS_PTR(node->property, FunctionProperty))
        json.add("property", AS_PTR(node->property, FunctionProperty));

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
    VARIANT_PTR(Variable);
    VARIANT_PTR(Literal);
    VARIANT_PTR(ListValue);
    VARIANT_PTR(InstanceList);
    VARIANT_PTR(UnresolvedIdentity);
    VARIANT_PTR(EnumValue);
    VARIANT_PTR(Unary);
    VARIANT_PTR(Binary);
    VARIANT_PTR(PropertyIndex);
    VARIANT_PTR(Match);
    VARIANT_PTR(InvalidValue);

    throw json_serialisation_error("Could not serialise Expression variant.");
};

string to_json(const Statement &node, const size_t &depth)
{
    VARIANT(Expression);
    VARIANT_PTR(CodeBlock);

    throw json_serialisation_error("Could not serialise Statement variant.");
};
