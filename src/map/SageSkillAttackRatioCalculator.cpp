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
		case EL_TORNADO_JG:
			add_chain_lighting_special_effects(target);
			return calculate_el_action_2_atk_ratio(skill_lv);
		case EL_ROCK_CRUSHER_JG:
			add_rock_crusher_special_effects(target);
			return calculate_el_action_2_atk_ratio(skill_lv);
		case EL_WATER_SCREW_JG:
			add_water_blast_special_effects(target);
			return calculate_el_action_2_atk_ratio(skill_lv);
		case EF_FIRE_BOMB_JG:
			add_fire_bomb_special_effects(target);
			return calculate_el_action_2_atk_ratio(skill_lv);
		case JG_EL_ACTION:
			return calculate_el_action_2_atk_ratio(skill_lv);
		default:
			return 0;
			break;
	}
}


void SageSkillAttackRatioCalculator::add_fire_bomb_special_effects(struct block_list *target)
{
	clif_specialeffect(target, 1270, AREA);
	clif_specialeffect(target, 49, AREA);
	clif_specialeffect(target, 50, AREA);
}

void SageSkillAttackRatioCalculator::add_water_blast_special_effects(struct block_list *target)
{
	clif_specialeffect(target, 1271, AREA);
	clif_specialeffect(target, 1041, AREA);
}

void SageSkillAttackRatioCalculator::add_chain_lighting_special_effects(struct block_list *target)
{
	clif_specialeffect(target, 1272, AREA);
	clif_specialeffect(target, EF_THUNDERSTORM, AREA);
}

void SageSkillAttackRatioCalculator::add_rock_crusher_special_effects(struct block_list *target)
{
	clif_specialeffect(target, 1273, AREA);
	clif_specialeffect(target, 950, AREA);
}

int SageSkillAttackRatioCalculator::calculate_el_action_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 50;
			break;
		case 2:
			ratio = 150;
			break;
		case 3:
			ratio = 250;
			break;
		case 4:
			ratio = 350;
			break;
		case 5:
			ratio = 450;
			break;
		}
	return ratio;
}

int SageSkillAttackRatioCalculator::calculate_el_action_2_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 0;
			break;
		case 2:
			ratio = 100;
			break;
		case 3:
			ratio = 200;
			break;
		case 4:
			ratio = 300;
			break;
		case 5:
			ratio = 400;
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
