#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class AcolyteSkillAtkRatioCalculator
{
	private:

	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv);

	private:
		static int calculate_sacred_wave_atk_ratio(int skill_lv);
		static int calculate_holy_light_atk_ratio(int skill_lv);
		static void add_sacred_wave_special_effects(struct block_list *target);
};
