#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class SageSkillAttackRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus);

	private:
		static int calculate_earth_spikes_atk_ratio(int skill_lv);
        static int calculate_vermillion_atk_ratio(int skill_lv);
        static int calculate_storm_gust_atk_ratio(int skill_lv);
        static int calculate_meteor_storm_atk_ratio(int skill_lv);
		static int calculate_soul_burn_atk_ratio(int skill_lv);
		static int calculate_el_action_atk_ratio(int skill_lv);
		static int calculate_el_action_2_atk_ratio(int skill_lv);
	static int calculate_el_action_3_atk_ratio(int skill_lv);
		static void add_fire_bomb_special_effects(struct block_list *target);
		static void add_water_blast_special_effects(struct block_list *target);
		static void add_chain_lighting_special_effects(struct block_list *target);
		static void add_rock_crusher_special_effects(struct block_list *target);
};
