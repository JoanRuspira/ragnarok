#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class PriestAdditionalEffectsCalculator
{
    public:
        static void apply_unholy_cross_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
};
