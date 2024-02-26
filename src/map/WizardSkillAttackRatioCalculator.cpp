#include "WizardSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int WizardSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case SK_WZ_ICEBERG:
			add_iceberg_special_effects(target);
			return calculate_bolt2_attack(skill_lv);
			break;
		case SK_WZ_STALAGMITE:
			add_stalagmite_special_effects(target);
			return calculate_bolt2_attack(skill_lv);
			break;
		case SK_WZ_CRIMSONROCK:
			return calculate_bolt2_attack(skill_lv);
			break;
		case SK_SO_SHADOWBOMB:
			clif_specialeffect(target, 1347, AREA);
			clif_specialeffect(target, 894, AREA);
			return calculate_bolt3_attack(skill_lv);
			break;
		case SK_WZ_THUNDERSTORM:
			clif_specialeffect(target, 30, AREA);
			return calculate_bolt2_attack(skill_lv);
			break;
		case SK_SO_PHANTOMSPEAR:
			clif_specialeffect(target, 1353, AREA);
			clif_specialeffect(target, 930, AREA);
			return calculate_bolt3_attack(skill_lv);
			break;
		case SK_WZ_ILLUSIONARYBLADES:
			return calculate_bolt2_attack(skill_lv);
			break;
		case SK_WZ_ICONOFSIN:
			add_corrupt_special_effects(target);
			return calculate_corrupt_atk_ratio(skill_lv);
			break;
		case SK_WZ_EXTREMEVACUUM:
			add_extreme_vacuum_special_effects(target);
			return calculate_extreme_vacuum_atk_ratio(skill_lv);
			break;
		case SK_WZ_REALITYBREAKER:
			add_land_of_evil_special_effects(target);
			return calculate_land_of_evil_atk_ratio(skill_lv);
			break;
		case SK_SO_ASTRALSTRIKE:
			clif_specialeffect(target, 1373, AREA); //new_banishingpoint_01
			clif_specialeffect(target, 1374, AREA); //new_banishingpoint_05
			return calculate_astral_strike_atk_ratio(skill_lv);
			break;
		case SK_SO_DOOM:
			clif_specialeffect(target, 1351, AREA); //new_armscannon_07_clock_up
			return calculate_land_of_evil_atk_ratio(skill_lv);
			break;
		case SK_SO_DOOM_GHOST:
			clif_specialeffect(target, 1352, AREA); //new_armscannon_07_clock_down
			return calculate_land_of_evil_atk_ratio(skill_lv);
			break;
		case SK_SO_DIAMONDDUST:
			clif_specialeffect(target, 1282, AREA); 
			clif_specialeffect(target, 1283, AREA); 
			clif_specialeffect(target, 1284, AREA); 
			return calculate_diamond_dust_atk_ratio(skill_lv);
			break;
		case SK_MG_VOIDEXPANSION:
			return calculate_void_expansion_atk_ratio(skill_lv);
			break;
		case SK_WZ_MAGICCRASHER:
			add_magic_crasher_special_effects(target);
			return calculate_magic_crasher_atk_ratio(skill_lv);
			break;
		case SK_WZ_PSYCHICWAVE:
			return calculate_psychic_wave_atk_ratio(skill_lv);
			break;
		case SK_SO_TETRAVORTEX_FIRE:
			clif_specialeffect(target, 1266, AREA); 
			return calculate_tetra_vortex_atk_ratio(skill_lv);
			break;
		case SK_SO_TETRAVORTEX_WATER:
		case SK_SO_TETRAVORTEX_WIND:
		case SK_SO_TETRAVORTEX_GROUND:
			return calculate_tetra_vortex_atk_ratio(skill_lv);
			break;
		case SK_SO_TETRAVORTEX_NEUTRAL:
    		clif_specialeffect(target, EF_STORMKICK2, AREA);
    		clif_specialeffect(target, EF_THUNDERSTORM2, AREA);
			return calculate_tetra_vortex_atk_ratio(skill_lv)*2;
			break;
		default:
			return 0;
			break;
	}
}

int WizardSkillAttackRatioCalculator::calculate_tetra_vortex_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 25;
			break;
		case 2:
			ratio = 50;
			break;
		case 3:
			ratio = 75;
			break;
		case 4:
			ratio = 100;
			break;
		case 5:
			ratio = 125;
			break;
		}
	return ratio;
}


int WizardSkillAttackRatioCalculator::calculate_void_expansion_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 10;
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

int WizardSkillAttackRatioCalculator::calculate_bolt2_attack(int skill_lv)
{
	int ratio = 50;
	return ratio;
}

int WizardSkillAttackRatioCalculator::calculate_bolt3_attack(int skill_lv)
{
	int ratio = 70;
	return ratio;
}

void WizardSkillAttackRatioCalculator::add_land_of_evil_special_effects(struct block_list *target)
{
    clif_specialeffect(target, 1350, AREA);
}

void WizardSkillAttackRatioCalculator::add_corrupt_special_effects(struct block_list *target)
{
    clif_specialeffect(target, 1234, AREA);
    clif_specialeffect(target, 124, AREA);
    clif_specialeffect(target, 1277, AREA);
}

void WizardSkillAttackRatioCalculator::add_extreme_vacuum_special_effects(struct block_list *target)
{
    clif_specialeffect(target, 1323, AREA);
}

void WizardSkillAttackRatioCalculator::add_magic_crasher_special_effects(struct block_list *target)
{
    clif_specialeffect(target, 1380, AREA); //new_dragonbreath_01_clock
}

int WizardSkillAttackRatioCalculator::calculate_astral_strike_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = -50;
			break;
		case 2:
			ratio = 150;
			break;
		case 3:
			ratio = 350;
			break;
		case 4:
			ratio = 550;
			break;
		case 5:
			ratio = 750;
			break;
		}
	return ratio;
}

int WizardSkillAttackRatioCalculator::calculate_land_of_evil_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 25;
			break;
		case 2:
			ratio = 125;
			break;
		case 3:
			ratio = 225;
			break;
		case 4:
			ratio = 325;
			break;
		case 5:
			ratio = 425;
			break;
		}
	return ratio;
}

int WizardSkillAttackRatioCalculator::calculate_diamond_dust_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 100;
			break;
		case 2:
			ratio = 300;
			break;
		case 3:
			ratio = 500;
			break;
		case 4:
			ratio = 700;
			break;
		case 5:
			ratio = 900;
			break;
		}
	return ratio;
}

int WizardSkillAttackRatioCalculator::calculate_psychic_wave_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = -75;
			break;
		case 2:
			ratio = -50;
			break;
		case 3:
			ratio = -25;
			break;
		case 4:
			ratio = 0;
			break;
		case 5:
			ratio = 25;
			break;
		}
	return ratio;
}
int WizardSkillAttackRatioCalculator::calculate_extreme_vacuum_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 25;
			break;
		case 2:
			ratio = 125;
			break;
		case 3:
			ratio = 225;
			break;
		case 4:
			ratio = 325;
			break;
		case 5:
			ratio = 425;
			break;
		}
	return ratio;
}

int WizardSkillAttackRatioCalculator::calculate_magic_crasher_atk_ratio(int skill_lv)
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

int WizardSkillAttackRatioCalculator::calculate_corrupt_atk_ratio(int skill_lv)
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

void WizardSkillAttackRatioCalculator::add_iceberg_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_HYOUSYOURAKU, AREA);
}

void WizardSkillAttackRatioCalculator::add_stalagmite_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_KEEPING, AREA);
}

