#include "intrinsic.h"
#include "span.h"

namespace Intrinsic
{
    // FIXME: Give intrinsics some kind of 'intrinsic spans'. Eventually, if the compiler tries to access the span of an
    //        intrinsic, that should be a compiler error, as that's never going to make a useful error message for the user.
    //        For the purposes of building the compiler however, it would be useful to not have to deal with those compiler
    //        errors yet. (let alone the kind of memory segmentation errors that can occur due to null spans).

    ptr<PrimitiveType> type_str = ptr<PrimitiveType>(new PrimitiveType({"str", "std::string"}));
    ptr<PrimitiveType> type_num = ptr<PrimitiveType>(new PrimitiveType({"num", "double"}));
    ptr<PrimitiveType> type_int = ptr<PrimitiveType>(new PrimitiveType({"int", "int"}));
    ptr<PrimitiveType> type_amt = ptr<PrimitiveType>(new PrimitiveType({"amt", "int"}));
    ptr<PrimitiveType> type_bool = ptr<PrimitiveType>(new PrimitiveType({"bool", "bool"}));

    ptr<PrimitiveType> type_none = ptr<PrimitiveType>(new PrimitiveType({"none", "void"}));
    ptr<PrimitiveValue> none_val = ptr<PrimitiveValue>(new PrimitiveValue({0, type_none}));

    // FIXME: Is it worth adding an 'IntrinsicEntity' type?
    ptr<EntityType> entity_player = ptr<EntityType>(new EntityType({Span(), "Player"}));

    // FIXME: As of writing, the `player` parameter is never declared in the state's scope, as normally the parser would
    //        declare it. This may not be an issue for properties that are implemented intrinsically, but worth noting.
    //        Could it potentially be worth adding an `IntrinsicProperty` type?
    ptr<StateProperty> state_player_number = ptr<StateProperty>(new StateProperty({
        Span(),                                                           // span
        "number",                                                         // identity
        type_amt,                                                         // pattern
        CREATE(Scope),                                                    // scope
        {ptr<Variable>(new Variable({Span(), "player", entity_player}))}, // parameters
        {}                                                                // initial_value
    }));

    ptr<EntityType> entity_game = ptr<EntityType>(new EntityType({Span(), "Game"}));

    ptr<Variable> variable_game = ptr<Variable>(new Variable({Span(), "game", entity_game, false}));

    ptr<StateProperty> state_game_players = ptr<StateProperty>(new StateProperty({
        Span(),                                                       // span
        "players",                                                    // identity
        ptr<ListType>(new ListType({entity_player, {}})),             // pattern
        CREATE(Scope),                                                // scope
        {ptr<Variable>(new Variable({Span(), "game", entity_game}))}, // parameters
        {}                                                            // initial_value
    }));
}