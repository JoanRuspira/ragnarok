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
	static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv);

private:
	static int calculate_bash_atk_ratio(int skill_lv);
	static int calculate_magnum_break_atk_ratio(int skill_lv);
	static int calculate_spear_stab_atk_ratio(int skill_lv, struct block_list *target);
	static int calculate_traumatic_blow_atk_ratio(int skill_lv);
	static void add_spear_stab_special_effects(struct block_list* src, struct block_list *target);
	static void add_traumatic_blow_special_effects(struct block_list* src, struct block_list *target);

};
