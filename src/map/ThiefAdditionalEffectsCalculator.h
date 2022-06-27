#pragma once
#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class ThiefAdditionalEffectsCalculator
{
public:
	static void apply_envenom_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
	static void apply_throw_stone_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
	static void apply_steal_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv, int &skill);
	static void apply_sand_attack_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
};
