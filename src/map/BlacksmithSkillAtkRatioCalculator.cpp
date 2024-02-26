#include "BlacksmithSkillAtkRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "clif.hpp"

/**
 * ATK ratio calculater for Blacksmith skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int BlacksmithSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case SK_BS_CARTSHRAPNEL:
			return calculate_cart_shrapnel_atk_ratio(skill_lv,  sstatus->dex);
			break;
		case SK_MC_CARTBRUME:
			return calculate_cart_brume_atk_ratio(skill_lv,  sstatus->int_);
			break;
		case SK_MC_CARTFIREWORKS:
			return calculate_cart_brume_atk_ratio(skill_lv,  sstatus->int_);
			break;
		case SK_BS_CARTCANNON:
			return calculate_cart_cannon_atk_ratio(skill_lv,  sstatus->dex);
			break;
		case SK_BS_AXEBOOMERANG:
			return calculate_axe_boomerang_atk_ratio(skill_lv);
			break;
		case SK_BS_AXETORNADO:
			return calculate_axe_tornado_atk_ratio(skill_lv, sstatus->luk);
			break;
		case SK_BS_HAMMERFALL:
			add_hammerfall_special_effects(target);
			return calculate_hammerfall_atk_ratio(skill_lv, sstatus->str);
			break;
		case SK_MS_HAMMERDOWNPROTOCOL:
			clif_specialeffect(target, EF_SPRINGTRAP, AREA);
			return calculate_hammerdown_protocol_atk_ratio(skill_lv, sstatus->str, sstatus->dex);
			break;
		case SK_MS_POWERSWING:
			return calculate_power_swing_atk_ratio(skill_lv, sstatus->str, sstatus->luk);
			break;
		case SK_BS_CROWDCONTROLSHOT:
			clif_soundeffectall(target, "crowdcontrol.wav", 0, AREA);
			return calculate_crowd_control_shot_atk_ratio(skill_lv, sstatus->dex);
			break;
		case SK_MS_TRIGGERHAPPYCART:
			clif_specialeffect(src, 1137, AREA);
			clif_specialeffect(target, 1150, AREA);
			return calculate_trigger_happy_cart_atk_ratio(skill_lv, sstatus->dex);
			break;
		case SK_MS_CARTTERMINATION:
			return calculate_cart_termination_atk_ratio(skill_lv,  sstatus->dex);
			break;
		default:
			return 0;
			break;
	}
}


void BlacksmithSkillAtkRatioCalculator::add_hammerfall_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_CRASHEARTH, AREA);
}


int BlacksmithSkillAtkRatioCalculator::calculate_cart_termination_atk_ratio(int skill_lv, int dex)
{
	int ratio = 0;
	
	switch (skill_lv) {
		case 1:
			ratio = 100;
			break;
		case 2:
			ratio = 200;
			break;
		case 3:
			ratio = 300;
			break;
		case 4:
			ratio = 400;
			break;
		case 5:
			ratio = 500;
			break;
		}
	return ratio + dex;
}

int BlacksmithSkillAtkRatioCalculator::calculate_trigger_happy_cart_atk_ratio(int skill_lv, int dex)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 200;
			break;
		case 2:
			ratio = 300;
			break;
		case 3:
			ratio = 400;
			break;
		case 4:
			ratio = 500;
			break;
		case 5:
			ratio = 600;
			break;
		}
	return ratio + (dex);
}

int BlacksmithSkillAtkRatioCalculator::calculate_crowd_control_shot_atk_ratio(int skill_lv, int dex)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 150;
			break;
		case 2:
			ratio = 200;
			break;
		case 3:
			ratio = 250;
			break;
		case 4:
			ratio = 300;
			break;
		case 5:
			ratio = 350;
			break;
		}
	return ratio + (dex/3);
}

int BlacksmithSkillAtkRatioCalculator::calculate_hammerdown_protocol_atk_ratio(int skill_lv, int str, int dex)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 300;
			break;
		case 2:
			ratio = 400;
			break;
		case 3:
			ratio = 500;
			break;
		case 4:
			ratio = 600;
			break;
		case 5:
			ratio = 700;
			break;
		}
	return ratio + (str/3) + (dex/3);
}

int BlacksmithSkillAtkRatioCalculator::calculate_power_swing_atk_ratio(int skill_lv, int str, int luk)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 150;
			break;
		case 2:
			ratio = 250;
			break;
		case 3:
			ratio = 350;
			break;
		case 4:
			ratio = 450;
			break;
		case 5:
			ratio = 550;
			break;
		}
	return ratio + (str/3) + (luk/3);
}

int BlacksmithSkillAtkRatioCalculator::calculate_hammerfall_atk_ratio(int skill_lv, int str)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 30;
			break;
		case 2:
			ratio = 100;
			break;
		case 3:
			ratio = 180;
			break;
		case 4:
			ratio = 240;
			break;
		case 5:
			ratio = 300;
			break;
		}
	return ratio + (str/3);
}


int BlacksmithSkillAtkRatioCalculator::calculate_axe_tornado_atk_ratio(int skill_lv, int luk)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 30;
			break;
		case 2:
			ratio = 100;
			break;
		case 3:
			ratio = 180;
			break;
		case 4:
			ratio = 240;
			break;
		case 5:
			ratio = 300;
			break;
		}
	return ratio + (luk/3);
}

int BlacksmithSkillAtkRatioCalculator::calculate_axe_boomerang_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 30;
			break;
		case 2:
			ratio = 100;
			break;
		case 3:
			ratio = 180;
			break;
		case 4:
			ratio = 240;
			break;
		case 5:
			ratio = 300;
			break;
		}
	return ratio;
}


int BlacksmithSkillAtkRatioCalculator::calculate_cart_cannon_atk_ratio(int skill_lv, int dex)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 30;
			break;
		case 2:
			ratio = 100;
			break;
		case 3:
			ratio = 180;
			break;
		case 4:
			ratio = 240;
			break;
		case 5:
			ratio = 300;
			break;
		}
	return ratio + (dex/3);
}


int BlacksmithSkillAtkRatioCalculator::calculate_cart_shrapnel_atk_ratio(int skill_lv, int dex)
{
	int ratio = 0;
	
	switch (skill_lv) {
		case 1:
			ratio = -71 + (dex/9);
			break;
		case 2:
			ratio = -42 + (dex/9);
			break;
		case 3:
			ratio = -13 + (dex/9);
			break;
		case 4:
			ratio = 16 + (dex/9);
			break;
		case 5:
			ratio = 45 + (dex/9);
			break;
	}
	return ratio;
}


int BlacksmithSkillAtkRatioCalculator::calculate_cart_brume_atk_ratio(int skill_lv, int intelligence)
{
	int ratio = 0;
	
	switch (skill_lv) {
		case 1:
			ratio = -71 + (intelligence/6);
			break;
		case 2:
			ratio = -42 + (intelligence/6);
			break;
		case 3:
			ratio = -13 + (intelligence/6);
			break;
		case 4:
			ratio = 16 + (intelligence/6);
			break;
		case 5:
			ratio = 45 + (intelligence/6);
			break;
	}
	return ratio;
}
