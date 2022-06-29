#include "MerchntSkillAtkRatioCalculator.h"

/**
 * ATK ratio calculater for Thief skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int MerchntSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv)
{
	switch (skill_id) {
	case MC_MAMMONITE:
		return calculate_mammonite_atk_ratio(skill_lv);
		break;
	case MC_CARTREVOLUTION:
		return calculate_cart_revolution_atk_ratio(skill_lv);
		break;
	case MC_FIREWORKS:
		add_cart_fireworks_special_effects(target);
		return calculate_cart_fireworks_atk_ratio(skill_lv);
		break;
	default:
		return 0;
		break;
	}
}

void MerchntSkillAtkRatioCalculator::add_cart_fireworks_special_effects(struct block_list* src)
{
	clif_specialeffect(src, EF_CARTTER, AREA);
	// clif_specialeffect(target, EF_POISONATTACK, AREA);
}

int MerchntSkillAtkRatioCalculator::calculate_mammonite_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
	case 1:
		ratio = -30;
		break;
	case 2:
		ratio = 40;
		break;
	case 3:
		ratio = 110;
		break;
	case 4:
		ratio = 180;
		break;
	case 5:
		ratio = 250;
		break;
	}
	return ratio;
}

int MerchntSkillAtkRatioCalculator::calculate_cart_revolution_atk_ratio(int skill_lv)
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

int MerchntSkillAtkRatioCalculator::calculate_cart_fireworks_atk_ratio(int skill_lv)
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
