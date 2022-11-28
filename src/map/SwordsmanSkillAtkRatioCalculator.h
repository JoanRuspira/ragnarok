#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class SwordsmanSkillAtkRatioCalculator
{
	private:

	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus);

	private:
		static int calculate_bash_atk_ratio(int skill_lv);
		static int calculate_magnum_break_atk_ratio(int skill_lv);
		static int calculate_spear_stab_atk_ratio(int skill_lv, struct block_list *target);
		static int calculate_traumatic_blow_atk_ratio(int skill_lv);
		static int calculate_spear_stab_normal_atk_ratio(int skill_lv);
		static int calculate_spear_stab_bleeding_atk_ratio(int skill_lv);
		static int calculate_lightning_strike_atk_ratio(int skill_lv, int intelligence);
		static void add_spear_stab_special_effects(struct block_list* src, struct block_list *target);
		static void add_traumatic_blow_special_effects(struct block_list* src, struct block_list *target);
		static void add_bash_special_effects(struct block_list *target);
		static void add_lightning_strike_special_effects(struct block_list *target);
		static void add_magnum_break_special_effects(struct block_list* src);
	
};
