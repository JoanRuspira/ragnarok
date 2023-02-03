#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class HunterSkillAttackRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus);

	private:
		static int calculate_slash_atk_ratio(int skill_lv, int intelligence);
		static int calculate_cyclonic_charge_atk_ratio(int skill_lv);
		static void add_slash_special_effects(struct block_list *target);
};
