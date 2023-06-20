#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class BardAdditionalEffectsCalculator
{
    public:
        static void apply_tarot_cards_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
        static void apply_great_echo_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
        static void apply_severe_rainstorm_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
};
