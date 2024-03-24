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
		case SK_WZ_EARTHSTRAIN:
			return calculate_earth_spikes_atk_ratio(skill_lv);
			break;
        case SK_WZ_LORDOFVERMILION:
			return calculate_vermillion_atk_ratio(skill_lv);
			break;
		case SK_WZ_STORMGUST:
			return calculate_storm_gust_atk_ratio(skill_lv);
			break;
		case SK_WZ_METEORSTORM:
			return calculate_meteor_storm_atk_ratio(skill_lv);
			break;
		case SK_SA_DRAINLIFE:
		case SK_SA_SOULBURN:
			return calculate_soul_burn_atk_ratio(skill_lv);
			break;
		case SK_SA_ICICLE:
			clif_specialeffect(target, 133, AREA);
			clif_specialeffect(target, 135, AREA);
			return calculate_el_action_atk_ratio(skill_lv);
			break;
		case SK_SA_WINDSLASH:
			return calculate_el_action_atk_ratio(skill_lv);
			break;
		case SK_SA_EARTHSPIKE:
			clif_specialeffect(target, EF_EARTHSPIKE, AREA);
			return calculate_el_action_atk_ratio(skill_lv);
			break;
		case SK_SA_FIREBALL:
			clif_specialeffect(target, 1254, AREA);
			return calculate_el_action_atk_ratio(skill_lv);
			break;
		case EL_TORNADO_JG:
			add_chain_lighting_special_effects(target);
			return calculate_el_action_2_atk_ratio(skill_lv);
		case SK_SA_ROCKCRUSHER:
			// add_rock_crusher_special_effects(target);
			return calculate_el_action_2_atk_ratio(skill_lv);
		case SK_SA_WATERBLAST:
			// add_water_blast_special_effects(target);
			return calculate_el_action_2_atk_ratio(skill_lv);
		case SK_SA_FIREBOMB:
			// add_fire_bomb_special_effects(target);
			return calculate_el_action_2_atk_ratio(skill_lv);
		case SK_SA_ELEMENTALACTION2:
			return calculate_el_action_2_atk_ratio(skill_lv);
		case SK_PF_INFERNO:
			clif_specialeffect(target, 1270, AREA);
			clif_specialeffect(target, EF_MAGMA_FLOW, AREA);
			return calculate_el_action_3_atk_ratio(skill_lv);
		case SK_PF_JUPITELTHUNDER:
			clif_specialeffect(target, 1272, AREA);
			return calculate_el_action_3_atk_ratio(skill_lv);
		case SK_PF_ROCKTOMB:
			clif_soundeffectall(target, "rocktomb.wav", 0, AREA);
    		clif_specialeffect(target, 1255, AREA);
			return calculate_el_action_3_atk_ratio(skill_lv);
		case SK_PF_HYDROPUMP:
			clif_soundeffectall(target, "hydropump.wav", 0, AREA);
			clif_specialeffect(target, 1271, AREA);
			clif_specialeffect(target, 1041, AREA);
			return calculate_el_action_3_atk_ratio(skill_lv);
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

int SageSkillAttackRatioCalculator::calculate_el_action_3_atk_ratio(int skill_lv)
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
	return ratio;
}
