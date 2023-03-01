#include "SageSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int SageSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case SO_EARTHGRAVE:
			return calculate_earth_spikes_atk_ratio(skill_lv);
			break;
        case WZ_VERMILION:
			return calculate_vermillion_atk_ratio(skill_lv);
			break;
		case WZ_STORMGUST:
			return calculate_storm_gust_atk_ratio(skill_lv);
			break;
		case WZ_METEOR:
			return calculate_meteor_storm_atk_ratio(skill_lv);
			break;
		case WL_DRAINLIFE:
		case PF_SOULBURN:
			return calculate_soul_burn_atk_ratio(skill_lv);
			break;
		case EL_ICE_NEEDLE:
		case EL_WIND_SLASH:
		case EL_STONE_HAMMER:
		case EL_FIRE_ARROW:
		case SO_EL_ACTION:
			return calculate_el_action_atk_ratio(skill_lv);
		default:
			return 0;
			break;
	}
}


int SageSkillAttackRatioCalculator::calculate_el_action_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 125;
			break;
		case 2:
			ratio = 225;
			break;
		case 3:
			ratio = 325;
			break;
		case 4:
			ratio = 425;
			break;
		case 5:
			ratio = 525;
			break;
		}
	return ratio;
}

int SageSkillAttackRatioCalculator::calculate_earth_spikes_atk_ratio(int skill_lv)
{
	return skill_lv*100;
}

int SageSkillAttackRatioCalculator::calculate_vermillion_atk_ratio(int skill_lv)
{
	return skill_lv*50;
}

int SageSkillAttackRatioCalculator::calculate_storm_gust_atk_ratio(int skill_lv)
{
	return skill_lv*20;
}

int SageSkillAttackRatioCalculator::calculate_meteor_storm_atk_ratio(int skill_lv)
{
	return skill_lv*5;
}

int SageSkillAttackRatioCalculator::calculate_soul_burn_atk_ratio(int skill_lv)
{
	return skill_lv*120;
}
