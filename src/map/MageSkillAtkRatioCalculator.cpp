#include "MageSkillAttackRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"

/**
 * ATK ratio calculater for Mage skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int MageSkillAtkRatioCalculator::calculate_skill_atk_ratio(int base_lv, int skill_id, int skill_lv)
{
	switch (skill_id) {
		/*case WZ_EARTHSPIKE:
			return calculate_earth_spike_attack(skill_lv);
			break;*/
		/*case MG_LIGHTNINGBOLT:
			return calculate_lightning_bolt_attack(skill_lv);
			break;*/
		case MG_COLDBOLT:
			return calculate_cold_bolt_attack(skill_lv);
			break;
		/*case MG_FIREBOLT:
			return calculate_fire_bolt_attack(skill_lv);
			break;
		case WZ_HEAVENDRIVE:
			return calculate_heavens_drive_attack(skill_lv);
			break;
		case MG_THUNDERSTORM:
			return calculate_thunderstorm_attack(skill_lv);
			break;
		case MG_SOULSTRIKE:
			return calculate_soul_strike_drive_attack(skill_lv);
			break;
		case NPC_DARKSTRIKE:
			return calculate_dark_strike_attack(skill_lv);
			break;
		case MG_UNDEADEMBRACE:
			return calculate_undead_embrace_attack(skill_lv);
			break;
		case MG_NAPALMBEAT:
			return calculate_napalm_beat_attack(skill_lv);
			break;*/
		default:
			return 0;
			break;
	}
}

int MageSkillAtkRatioCalculator::calculate_cold_bolt_attack(int skill_lv)
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

