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
		return calculate_cart_fireworks_atk_ratio(skill_lv, src);
		break;
	default:
		return 0;
		break;
	}
}

void MerchntSkillAtkRatioCalculator::add_cart_fireworks_special_effects(struct block_list* src)
{
	clif_specialeffect(src, EF_CARTTER, AREA);
}

int MerchntSkillAtkRatioCalculator::calculate_mammonite_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 80;
			break;
		case 2:
			ratio = 150;
			break;
		case 3:
			ratio = 230;
			break;
		case 4:
			ratio = 290;
			break;
		case 5:
			ratio = 350;
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

int MerchntSkillAtkRatioCalculator::calculate_cart_fireworks_atk_ratio(int skill_lv, struct block_list* src)
{
	int ratio = 0;
	struct status_data *status;
	status = status_get_status_data(src);
	
	switch (skill_lv) {
		case 1:
			ratio = -71 + (status->int_/3);
			break;
		case 2:
			ratio = -42 + (status->int_/3);
			break;
		case 3:
			ratio = -13 + (status->int_/3);
			break;
		case 4:
			ratio = 16 + (status->int_/3);
			break;
		case 5:
			ratio = 45 + (status->int_/3);
			break;
		}
	return ratio;
}
