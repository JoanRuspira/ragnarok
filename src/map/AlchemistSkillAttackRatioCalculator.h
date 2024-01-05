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
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, struct status_data* tstatus);

	private:
		static int calculate_basilisk_1_attack_ratio(int skill_lv);
		static int calculate_basilisk_2_attack_ratio(int skill_lv);
		static int calculate_demonstration_attack_ratio(int skill_lv, int intelligence);
		static int calculate_acid_terror_attack_ratio(int skill_lv, int dex);
		static int calculate_bomb_attack_ratio(int skill_lv, int dex);
		static int calculate_wild_thorns_attack_ratio(int skill_lv, int intelligence);
		static int calculate_incendiary_bomb_atk_ratio(int skill_lv, int intelligence);
		static int calculate_acid_bomb_atk_ratio(int skill_lv, int intelligence, int dex, int vit);
		static void calculate_basilisk_1_special_effects(struct block_list *target);
		static void calculate_basilisk_2_special_effects(struct block_list *target);
		static void calculate_beholder_1_special_effects(struct block_list *target);
		static void calculate_beholder_2_special_effects(struct block_list *target);
		static void calculate_acid_terror_special_effects(struct block_list *target);

};
