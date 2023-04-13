#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class AlchemistAdditionalEffectsCalculator
{
    public:
        static void apply_acid_terror_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
        static void apply_bomb_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
};
