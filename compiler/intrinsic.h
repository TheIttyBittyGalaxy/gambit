#pragma once
#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "apm.h"
#include "utilty.h"
using namespace std;

namespace Intrinsic
{
    extern ptr<PrimitiveType> type_str;
    extern ptr<PrimitiveType> type_num;
    extern ptr<PrimitiveType> type_amt;
    extern ptr<PrimitiveType> type_int;
    extern ptr<PrimitiveType> type_bool;

    extern ptr<PrimitiveType> type_none;
    extern ptr<PrimitiveValue> none_val;

    extern ptr<EntityType> entity_player;
    extern ptr<StateProperty> state_player_number;

    extern ptr<EntityType> entity_game;
    extern ptr<Variable> variable_game;
    extern ptr<StateProperty> state_game_players;
}

#endif