#pragma once
#include "clif.hpp"

class KnightSkillAtkRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus);

	private:
		static int calculate_auto_counter_atk_ratio(int skill_lv);
		static int calculate_bowling_bash_atk_ratio(int skill_lv, struct block_list *target);
		static int calculate_bowling_bash_normal_atk_ratio(int skill_lv);
		static int calculate_bowling_bash_bleeding_atk_ratio(int skill_lv);
		static int calculate_spear_boomerang_atk_ratio(int skill_lv);
		static int calculate_pierce_atk_ratio(int skill_lv);
		static int calculate_brandish_spear_atk_ratio(int skill_lv, int vit);
		static int calculate_wind_cutter_atk_ratio(int skill_lv, int intelligence);
		static int calculate_sonic_wave_atk_ratio(int skill_lv, int intelligence);
		static int calculate_smite_atk_ratio(int skill_lv);
		static int calculate_reckoning_atk_ratio(int skill_lv);
		static int calculate_a_hundred_spears_atk_ratio(int skill_lv);
		static void add_auto_counter_special_effects(struct block_list* src, struct block_list *target);
		static void add_smite_special_effects(struct block_list *target);
};
