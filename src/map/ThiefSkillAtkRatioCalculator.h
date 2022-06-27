#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class ThiefSkillAtkRatioCalculator
{
private:

public:
	static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv);
	
private:
	static int calculate_bash_atk_ratio(int skill_lv);
	static int calculate_envenom_atk_ratio(int skill_lv);
	static void add_envenom_special_effects(struct block_list* src, struct block_list *target);
};
