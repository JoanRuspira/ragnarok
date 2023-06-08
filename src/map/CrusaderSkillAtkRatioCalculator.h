#pragma once
#include "clif.hpp"

class CrusaderSkillAtkRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus);

	private:
		static int calculate_holy_cross_atk_ratio(int skill_lv, int intelligence);
		static int calculate_grand_cross_atk_ratio(int skill_lv);
		static int calculate_rapid_smiting_atk_ratio(int skill_lv);
		static int calculate_shield_slam_atk_ratio(int skill_lv);
		static void add_shield_slam_special_effects(struct block_list *target);
};

