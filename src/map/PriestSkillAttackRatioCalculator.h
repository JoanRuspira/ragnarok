#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class PriestSkillAttackRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus);

	private:
		static int calculate_spiritu_sancti_atk_ratio(int skill_lv);
		static int calculate_judex_atk_ratio(int skill_lv);
		static int calculate_magnus_exorcismus_atk_ratio(int skill_lv);
		static int calculate_unholy_cross_atk_ratio(int skill_lv, int intelligence);
		static int calculate_duple_light_melee_atk_ratio(int skill_lv);
		static int calculate_duple_light_magic_atk_ratio(int skill_lv);
		static int calculate_sententia_atk_ratio(int skill_lv);
		static int calculate_benedictio_atk_ratio(int skill_lv);
		static int calculate_diabolis_cruciatus_atk_ratio(int skill_lv);
		static int calculate_penitentia_atk_ratio(int skill_lv);
		static void add_spiritu_sancti_special_effects(struct block_list *target);
		static void add_judex_special_effects(struct block_list *target);
		static void add_unholy_cross_special_effects(struct block_list *target);
		static void add_duple_liight_melee_special_effects(struct block_list *target);
		static void add_duple_liight_magic_special_effects(struct block_list *target);
};
