#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class BardSkillAttackRatioCalculator
{
	public:
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus);

	private:
		static int calculate_melody_strike_atk_ratio(int skill_lv);
		static int calculate_great_echo_atk_ratio(int skill_lv);
		static int calculate_metallic_sound_atk_ratio(int skill_lv);
		static int calculate_tarot_cards_atk_ratio(int skill_lv);
		static void add_melody_strike_special_effects(struct block_list *target);
		static void add_great_echo_special_effects(struct block_list *target);
};
