#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class MerchntSkillAtkRatioCalculator
{
private:

public:
	static int calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus);
	static void add_cart_fireworks_special_effects(struct block_list *src);
	static void add_cart_quake_effects(struct block_list* src);
	static void add_cart_cyclone_special_effects(struct block_list* src);
	static void add_cart_brume_special_effects(struct block_list* src);
	static void add_cart_sharpnel_special_effects(struct block_list* src);
	
private:
	static int calculate_mammonite_atk_ratio(int skill_lv);
	static int calculate_cart_revolution_atk_ratio(int skill_lv);
	static int calculate_cart_fireworks_atk_ratio(int skill_lv, struct block_list* src);
	static int calculate_cart_quake_atk_ratio(int skill_lv, int intelligence);
};
