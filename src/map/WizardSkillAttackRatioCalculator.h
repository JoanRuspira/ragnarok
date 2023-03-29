#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class WizardSkillAttackRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus);

	private:
		static int calculate_bolt2_attack(int skill_lv);
		static int calculate_corrupt_atk_ratio(int skill_lv);
		static int calculate_extreme_vacuum_atk_ratio(int skill_lv);
		static int calculate_land_of_evil_atk_ratio(int skill_lv);
		static int calculate_void_expansion_atk_ratio(int skill_lv);
		static void add_iceberg_special_effects(struct block_list *target);
		static void add_stalagmite_special_effects(struct block_list *target);
		static void add_corrupt_special_effects(struct block_list *target);
		static void add_extreme_vacuum_special_effects(struct block_list *target);
		static void add_land_of_evil_special_effects(struct block_list *target);

};
