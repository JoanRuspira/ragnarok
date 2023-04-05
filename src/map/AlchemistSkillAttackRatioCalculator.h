#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class AlchemistSkillAttackRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus);

	private:
		static int calculate_basilisk_1_attack_ratio(int skill_lv);
		static int calculate_basilisk_2_attack_ratio(int skill_lv);
		static void calculate_basilisk_1_special_effects(struct block_list *target);
		static void calculate_basilisk_2_special_effects(struct block_list *target);
		static void calculate_beholder_1_special_effects(struct block_list *target);

};
