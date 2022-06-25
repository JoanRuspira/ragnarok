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
	static int calculate_skill_atk_ratio(int base_lv, int skill_id, int skill_lv);
	static void SwordsmanSkillAtkRatioCalculator::add_skill_special_effect(struct block_list* src, struct block_list *target, int skill_id);

private:
	static int calculate_bash_atk_ratio(int skill_lv);
	static int calculate_magnum_break_atk_ratio(int skill_lv);
	static int calculate_spear_stab_atk_ratio(int skill_lv);
};
