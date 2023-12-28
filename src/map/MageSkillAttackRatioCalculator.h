#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"
class MageSkillAtkRatioCalculator
{
	private:

	public:
		static int calculate_skill_atk_ratio(int base_lv, int skill_id, int skill_lv, struct block_list *target, bool is_spellfist);

	private:
		static int calculate_bolt_attack(int skill_lv);
		static int calculate_soul_strike_attack(int skill_lv);
		static int calculate_undead_embrace_attack(int skill_lv);
		static void add_undead_embrace_special_effects(struct block_list *target);
		static void add_earth_bolt_special_effects(struct block_list *target);
};
