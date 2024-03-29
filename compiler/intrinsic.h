#pragma once
#ifndef INTRINSICS_H
#define INTRINSICS_H

#include "apm.h"
#include "utilty.h"
using namespace std;

namespace Intrinsic
{
    extern ptr<IntrinsicType> type_str;
    extern ptr<IntrinsicType> type_num;
    extern ptr<IntrinsicType> type_amt;
    extern ptr<IntrinsicType> type_int;
    extern ptr<IntrinsicType> type_bool;

    extern ptr<IntrinsicType> type_none;
    extern ptr<IntrinsicValue> none_val;

    extern ptr<Entity> entity_player;
    extern ptr<StateProperty> state_player_number;
}

#endif