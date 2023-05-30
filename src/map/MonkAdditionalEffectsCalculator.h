#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class MonkAdditionalEffectsCalculator
{
    public:
        static void apply_excruciating_fist_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
};
