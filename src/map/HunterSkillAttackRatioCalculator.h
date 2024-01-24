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
		static int calculate_cyclonic_charge_atk_ratio(int skill_lv);
		static int calculate_magic_tomahawk_atk_ratio(int skill_lv, int matk);
		static int calculate_slash_atk_ratio(int skill_lv, int int_);
		static int calculate_blitz_beat_atk_ratio(int skill_lv, int agi);
		static int calculate_sharp_shooting_atk_ratio(int skill_lv, int dex);
		static int calculate_ullrs_magic_shot_atk_ratio(int skill_lv);
		static int calculate_zephyr_sniping_atk_ratio(int skill_lv);
		static void add_sharp_shooting_special_effects(struct block_list *target);
		static void add_slash_special_effects(struct block_list *target);
		static void add_hurricane_fury_special_effects(struct block_list *target);
		static void add_magic_tomahawk_special_effects(struct block_list *target);
};
