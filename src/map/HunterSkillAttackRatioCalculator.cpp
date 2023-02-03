#include "HunterSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int HunterSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case RA_WUGSTRIKE:
			add_slash_special_effects(target);
			return 0;
			break;
		case HT_HURRICANEFURY:
			return calculate_cyclonic_charge_atk_ratio(skill_lv);
		default:
			return 0;
			break;
	}
}

void HunterSkillAttackRatioCalculator::add_slash_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_SONIC_CLAW, AREA);
}


int HunterSkillAttackRatioCalculator::calculate_cyclonic_charge_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 0;
			break;
		case 2:
			ratio = 25;
			break;
		case 3:
			ratio = 50;
			break;
		case 4:
			ratio = 75;
			break;
		case 5:
			ratio = 100;
			break;
		}
	return ratio;
}
