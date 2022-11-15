#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class RogueAdditionalEffectsCalculator
{
    public:
        static void apply_back_stab_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
};
