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
	static int calculate_envenom_atk_ratio(int skill_lv);
	static int calculate_throw_stone_atk_ratio(int skill_lv);
	static int calculate_snatch_atk_ratio(int skill_lv);
	static int calculate_sand_attack_atk_ratio(int skill_lv);
	static int calculate_venom_knife_atk_ratio(int skill_lv);
	static int calculate_poison_react_atk_ratio(int skill_lv);
	static void add_envenom_special_effects(struct block_list* src, struct block_list *target);
	static void add_snatch_special_effects(struct block_list *target);
	static void add_venom_knife_special_effects(struct block_list* src, struct block_list *target);
	static void add_sand_attack_special_effects(struct block_list *target);
	static void add_throw_stone_special_effects(struct block_list *target);
};
