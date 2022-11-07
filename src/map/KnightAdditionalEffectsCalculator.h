#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class KnightAdditionalEffectsCalculator
{
    public:
        static void apply_smite_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
        static void apply_auto_counter_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
};
