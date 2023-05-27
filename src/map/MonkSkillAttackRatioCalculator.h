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
		static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, bool revealed_hidden_enemy);

	private:
		static int calculate_throw_spirit_sphere_atk_ratio(int skill_lv);
		static int calculate_occult_impact(int skill_lv, defType defence);
		static int calculate_ground_shaker_atk_ratio(int skill_lv, int str, bool revealed_hidden_enemy);
};
