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
	static void calculate_skill_additional_effect(struct block_list* src, struct block_list *bl, int skill_id,
		int skill_lv, int flag = 0, int skill_area_temp[8] = 0, t_tick tick = 0);
	static void SwordsmanSkillAtkRatioCalculator::add_skill_special_effect(struct block_list* src, struct block_list *target, int skill_id);

private:
	static int calculate_bash_atk_ratio(int skill_lv);
	static int calculate_magnum_break_atk_ratio(int skill_lv);
	static int calculate_spear_stab_atk_ratio(int skill_lv);
	static void calculate_bash_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
	static void calculate_spear_stab_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv, int flag, int skill_area_temp[8], t_tick tick);
};
