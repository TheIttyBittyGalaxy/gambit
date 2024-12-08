#include "apm.h"
#include "json.h"

#define SHORT_LITERALS

// MACROS

#define STRUCT_FIELD(field) json.add(#field, node.field);

#define STRUCT_PTR_FIELD(field) json.add(#field, node->field);

#define STRUCT_PTR_FIELD_IDENTITY(field) json.add(#field, node->field->identity);

#define VARIANT(T)   \
    if (IS(node, T)) \
        return to_json(AS(node, T), depth);

#define VARIANT_PTR(T)   \
    if (IS_PTR(node, T)) \
        return to_json(AS_PTR(node, T), depth);

#define VARIANT_PTR_IDENTITY(T) \
    if (IS_PTR(node, T))        \
        return to_json(AS_PTR(node, T)->identity, depth);

// PROGRAM

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

string to_json(const ptr<Scope> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Scope"));
    STRUCT_PTR_FIELD(lookup);
    json.close();
    return (string)json;
}

string to_json(const Scope::LookupValue &node, const size_t &depth)
{
    VARIANT_PTR(Scope::OverloadedIdentity);
    VARIANT_PTR(Procedure);
    VARIANT_PTR(Variable);

    VARIANT_PTR(StateProperty);
    VARIANT_PTR(FunctionProperty);

    VARIANT(Pattern);

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

string to_json(const ptr<Procedure> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Procedure"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(scope);
    STRUCT_PTR_FIELD(parameters);
    STRUCT_PTR_FIELD(body);
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
    STRUCT_PTR_FIELD(is_constant);
    json.close();
    return (string)json;
}

// LITERALS

string to_json(const UnresolvedLiteral &node, const size_t &depth)
{
    VARIANT_PTR(PrimitiveLiteral);
    VARIANT_PTR(ListLiteral);
    VARIANT_PTR(IdentityLiteral);
    VARIANT_PTR(OptionLiteral);

    throw json_serialisation_error("Could not serialise UnresolvedLiteral variant.");
}

#ifdef SHORT_LITERALS

string to_json(const ptr<PrimitiveLiteral> &node, const size_t &depth)
{
    return to_json(node->value, depth);
}

string to_json(const ptr<ListLiteral> &node, const size_t &depth)
{
    return to_json(node->values, depth);
}

string to_json(const ptr<IdentityLiteral> &node, const size_t &depth)
{
    return to_json("<" + node->identity + ">");
}

#else

string to_json(const ptr<PrimitiveLiteral> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("PrimitiveLiteral"));
    STRUCT_PTR_FIELD(value);
    json.close();
    return (string)json;
}

string to_json(const ptr<ListLiteral> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("ListLiteral"));
    STRUCT_PTR_FIELD(values);
    json.close();
    return (string)json;
}

string to_json(const ptr<IdentityLiteral> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("IdentityLiteral"));
    STRUCT_PTR_FIELD(identity);
    json.close();
    return (string)json;
}

#endif

string to_json(const ptr<OptionLiteral> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("OptionLiteral"));
    STRUCT_PTR_FIELD(literal);
    json.close();
    return (string)json;
}

// VALUES

string to_json(const ptr<PrimitiveValue> &node, const size_t &depth)
{
    if (IS(node->value, double))
        return to_json(AS(node->value, double));
    if (IS(node->value, int))
        return to_json(AS(node->value, int));
    if (IS(node->value, bool))
        return to_json(AS(node->value, bool));
    if (IS(node->value, string))
        return to_json(AS(node->value, string));

    throw json_serialisation_error("Could not serialise PrimitiveValue.");
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

string to_json(const ptr<EnumValue> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("EnumValue"));
    STRUCT_PTR_FIELD(identity);
    json.close();
    return (string)json;
}

// TYPES

string to_json(const ptr<PrimitiveType> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("PrimitiveType"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(cpp_identity);
    json.close();
    return (string)json;
}

string to_json(const ptr<ListType> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("ListType"));
    STRUCT_PTR_FIELD(list_of);
    STRUCT_PTR_FIELD(fixed_size);
    json.close();
    return (string)json;
}

string to_json(const ptr<EntityType> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("EntityType"));
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

// PROPERTIES

string to_json(const Property &node, const size_t &depth)
{
    VARIANT_PTR(IdentityLiteral);
    VARIANT_PTR(StateProperty);
    VARIANT_PTR(FunctionProperty);
    VARIANT_PTR(InvalidProperty);

    throw json_serialisation_error("Could not serialise Property variant.");
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

string to_json(const ptr<InvalidProperty> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InvalidProperty"));
    json.close();
    return (string)json;
}

// PATTERNS

string to_json(const Pattern &node, const size_t &depth)
{
    VARIANT(UnresolvedLiteral);
    VARIANT_PTR(PatternLiteral);

    VARIANT_PTR(AnyPattern);

    VARIANT_PTR(UnionPattern);

    VARIANT_PTR(PrimitiveValue);
    VARIANT_PTR_IDENTITY(EnumValue);

    VARIANT_PTR_IDENTITY(PrimitiveType);
    VARIANT_PTR(ListType);
    VARIANT_PTR_IDENTITY(EnumType);
    VARIANT_PTR_IDENTITY(EntityType);

    VARIANT_PTR(UninferredPattern);
    VARIANT_PTR(InvalidPattern);

    throw json_serialisation_error("Could not serialise Pattern variant.");
}

#ifdef SHORT_LITERALS

string to_json(const ptr<PatternLiteral> &node, const size_t &depth)
{
    return to_json(node->pattern, depth);
}

#else

string to_json(const ptr<PatternLiteral> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("PatternLiteral"));
    STRUCT_PTR_FIELD(pattern);
    json.close();
    return (string)json;
}

#endif

string to_json(const ptr<AnyPattern> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("AnyPattern"));
    json.close();
    return (string)json;
}

string to_json(const ptr<UnionPattern> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("UnionPattern"));
    STRUCT_PTR_FIELD(identity);
    STRUCT_PTR_FIELD(patterns);
    json.close();
    return (string)json;
}

string to_json(const ptr<UninferredPattern> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("UninferredPattern"));
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

// EXPRESSIONS

string to_json(const Expression &node, const size_t &depth)
{
    VARIANT(UnresolvedLiteral);
    VARIANT_PTR(ExpressionLiteral);

    VARIANT_PTR(PrimitiveValue);
    VARIANT_PTR(ListValue);
    VARIANT_PTR_IDENTITY(EnumValue);
    VARIANT_PTR_IDENTITY(Variable);

    VARIANT_PTR(Unary);
    VARIANT_PTR(Binary);

    VARIANT_PTR(InstanceList);
    VARIANT_PTR(ExpressionIndex);
    VARIANT_PTR(PropertyIndex);

    VARIANT_PTR(Call);

    VARIANT_PTR(ChooseExpression);

    VARIANT_PTR(IfExpression);
    VARIANT_PTR(MatchExpression);

    VARIANT_PTR(InvalidExpression);

    throw json_serialisation_error("Could not serialise Expression variant.");
};

#ifdef SHORT_LITERALS

string to_json(const ptr<ExpressionLiteral> &node, const size_t &depth)
{
    return to_json(node->expr, depth);
}

#else

string to_json(const ptr<ExpressionLiteral> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("ExpressionLiteral"));
    STRUCT_PTR_FIELD(expr);
    json.close();
    return (string)json;
}

#endif

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

string to_json(const ptr<InstanceList> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InstanceList"));
    STRUCT_PTR_FIELD(values);
    json.close();
    return (string)json;
}

string to_json(const ptr<ExpressionIndex> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("ExpressionIndex"));
    STRUCT_PTR_FIELD(subject);
    STRUCT_PTR_FIELD(index);
    json.close();
    return (string)json;
}

string to_json(const ptr<PropertyIndex> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("PropertyIndex"));
    STRUCT_PTR_FIELD(expr);

    // TODO: Instead of just printing out the identity, print out a signature that
    //       allows different overloads to be distinguished.
    if (IS_PTR(node->property, FunctionProperty))
        json.add("property", AS_PTR(node->property, FunctionProperty)->identity);
    else if (IS_PTR(node->property, StateProperty))
        json.add("property", AS_PTR(node->property, StateProperty)->identity);
    else
        STRUCT_PTR_FIELD(property)

    json.close();
    return (string)json;
}

string to_json(const ptr<Call> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("Call"));
    STRUCT_PTR_FIELD(callee);
    STRUCT_PTR_FIELD(arguments);
    json.close();
    return (string)json;
}

string to_json(const Call::Argument &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    STRUCT_FIELD(name);
    STRUCT_FIELD(value);
    json.close();
    return (string)json;
}

string to_json(const ptr<ChooseExpression> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("ChooseExpression"));
    STRUCT_PTR_FIELD(player);
    STRUCT_PTR_FIELD(choices);
    STRUCT_PTR_FIELD(prompt);
    json.close();
    return (string)json;
}

string to_json(const ptr<IfExpression> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("IfExpression"));
    STRUCT_PTR_FIELD(rules);
    STRUCT_PTR_FIELD(has_else);
    json.close();
    return (string)json;
}

string to_json(const IfExpression::Rule &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    STRUCT_FIELD(condition);
    STRUCT_FIELD(result);
    json.close();
    return (string)json;
}

string to_json(const ptr<MatchExpression> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("MatchExpression"));
    STRUCT_PTR_FIELD(subject);
    STRUCT_PTR_FIELD(rules);
    STRUCT_PTR_FIELD(has_else);
    json.close();
    return (string)json;
}

string to_json(const MatchExpression::Rule &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    STRUCT_FIELD(pattern);
    STRUCT_FIELD(result);
    json.close();
    return (string)json;
}

string to_json(const ptr<InvalidExpression> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("InvalidExpression"));
    json.close();
    return (string)json;
}

// STATEMENTS

string to_json(const Statement &node, const size_t &depth)
{
    VARIANT_PTR(IfStatement);
    VARIANT_PTR(ForStatement);
    VARIANT_PTR(LoopStatement);
    VARIANT_PTR(ReturnStatement);
    VARIANT_PTR(WinsStatement);
    VARIANT_PTR(DrawStatement);
    VARIANT_PTR(AssignmentStatement);
    VARIANT_PTR(VariableDeclaration);

    VARIANT_PTR(CodeBlock);
    VARIANT(Expression);

    throw json_serialisation_error("Could not serialise Statement variant.");
};

string to_json(const ptr<IfStatement> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("IfStatement"));
    STRUCT_PTR_FIELD(rules);
    STRUCT_PTR_FIELD(else_block);
    json.close();
    return (string)json;
}

string to_json(const IfStatement::Rule &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    STRUCT_FIELD(condition);
    STRUCT_FIELD(code_block);
    json.close();
    return (string)json;
}

string to_json(const ptr<ForStatement> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("ForStatement"));
    STRUCT_PTR_FIELD(variable);
    STRUCT_PTR_FIELD(range);
    STRUCT_PTR_FIELD(scope);
    STRUCT_PTR_FIELD(body);
    json.close();
    return (string)json;
}

string to_json(const ptr<LoopStatement> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("LoopStatement"));
    STRUCT_PTR_FIELD(scope);
    STRUCT_PTR_FIELD(body);
    json.close();
    return (string)json;
}

string to_json(const ptr<ReturnStatement> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("ReturnStatement"));
    STRUCT_PTR_FIELD(value);
    json.close();
    return (string)json;
}

string to_json(const ptr<WinsStatement> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("WinsStatement"));
    STRUCT_PTR_FIELD(player);
    json.close();
    return (string)json;
}

string to_json(const ptr<DrawStatement> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("DrawStatement"));
    json.close();
    return (string)json;
}

string to_json(const ptr<AssignmentStatement> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("AssignmentStatement"));
    STRUCT_PTR_FIELD(subject);
    STRUCT_PTR_FIELD(value);
    json.close();
    return (string)json;
}

string to_json(const ptr<VariableDeclaration> &node, const size_t &depth)
{
    JsonContainer json(depth);
    json.object();
    json.add("node", string("VariableDeclaration"));
    STRUCT_PTR_FIELD_IDENTITY(variable);
    STRUCT_PTR_FIELD(value);
    json.close();
    return (string)json;
}
