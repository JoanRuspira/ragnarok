#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class MonkSkillAttackRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, bool revealed_hidden_enemy, map_session_data *sd, status_change *sc);

	private:
		static int calculate_throw_spirit_sphere_atk_ratio(int skill_lv);
		static int calculate_occult_impact(int skill_lv, defType defence);
		static int calculate_ki_blast_atk_ratio(int skill_lv, int str);
		static int calculate_raging_triple_blow_atk_ratio(int skill_lv);
		static int calculate_chain_combo_atk_ratio(int skill_lv, bool is_using_mace);
		static int calculate_circular_fists_atk_ratio(int skill_lv, bool revealed_hidden_enemy);
		static int calculate_palm_strike_atk_ratio(int skill_lv);
		static int calculate_falling_fist_atk_ratio(int skill_lv);
		static int calculate_dash_punch_atk_ratio(int skill_lv);
		static int calculate_guillotine_fists_atk_ratio(int skill_lv);
		static void add_falling_fist_special_effects(struct block_list *target);
};
