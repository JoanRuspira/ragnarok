#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"
class SwordsmanAdditionalEffectsCalculator
{
public:
	static void apply_bash_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
	static void apply_spear_stab_additional_effect2(struct block_list* src, struct block_list *bl, int skill_lv);
	static void apply_spear_stab_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv, int flag, int skill_area_temp[8], t_tick tick);
};
