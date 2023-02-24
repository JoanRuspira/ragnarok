#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class WizardAdditionalEffectsCalculator
{
    public:
        static void apply_crimson_rock_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
        static void apply_iceberg_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
        static void apply_stalagmite_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
};
