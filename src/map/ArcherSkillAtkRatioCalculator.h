#pragma once
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class ArcherSkillAtkRatioCalculator
{
	private:

	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list* target, int base_lv, int skill_id, int skill_lv);

	private:
		static int calculate_double_strafe_atk_ratio(int base_lv, int skill_lv);
		static int calculate_arrow_shower_atk_ratio(int base_lv, int skill_lv);
		static int calculate_spiritual_strafe_atk_ratio(int skill_lv, struct block_list* src);
		static int calculate_charge_arrow_atk_ratio(int base_lv, int skill_lv);
		static void add_tranquilizer_shot_special_effects(struct block_list *target);
		static void add_paralyzing_shot_special_effects(struct block_list *target);
		static void add_arrow_shower_special_effects(struct block_list *target);
		static void add_spiritual_strafe_special_effects(struct block_list *target);
		static void add_charge_arrow_special_effects(struct block_list *target);

};

