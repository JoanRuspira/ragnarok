#include "AlchemistSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int AlchemistSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case HM_BASILISK_1:
		case AM_EL_ACTION:
			calculate_basilisk_1_special_effects(target);
			return calculate_basilisk_1_attack_ratio(skill_lv);
			break;
		case HM_BEHOLDER_1:
			calculate_beholder_1_special_effects(target);
			return calculate_basilisk_1_attack_ratio(skill_lv);
			break;
		case AM2_HOM_ACTION:
		case HFLI_SBR44:
			calculate_basilisk_2_special_effects(target);
			return calculate_basilisk_2_attack_ratio(skill_lv);
			break;
		case HM_BEHOLDER_2:
			calculate_beholder_2_special_effects(target);
			return calculate_basilisk_2_attack_ratio(skill_lv);
			break;
		case AM_DEMONSTRATION:
			return calculate_demonstration_attack_ratio(skill_lv, sstatus->int_);
			break;
		case AM_ACIDTERROR:
			calculate_acid_terror_special_effects(target);
			return calculate_acid_terror_attack_ratio(skill_lv, sstatus->dex);
			break;
		case GN_SPORE_EXPLOSION:
			return calculate_bomb_attack_ratio(skill_lv, sstatus->dex);
			break;
		case GN_WALLOFTHORN:
			return calculate_wild_thorns_attack_ratio(skill_lv, sstatus->int_);
			break;
		case GN_DEMONIC_FIRE:
			return calculate_incendiary_bomb_atk_ratio(skill_lv, sstatus->int_);
			break;
		default:
			return 0;
			break;
	}
}

int AlchemistSkillAttackRatioCalculator::calculate_incendiary_bomb_atk_ratio(int skill_lv, int intelligence)
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
	return ratio + intelligence;
}

int AlchemistSkillAttackRatioCalculator::calculate_wild_thorns_attack_ratio(int skill_lv, int intelligence)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = -80;
			break;
		case 2:
			ratio = -60;
			break;
		case 3:
			ratio = -40;
			break;
		case 4:
			ratio = -20;
			break;
		case 5:
			ratio = 0;
			break;
	}
	return ratio + (intelligence);
}

int AlchemistSkillAttackRatioCalculator::calculate_bomb_attack_ratio(int skill_lv, int dex)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 250;
			break;
		case 2:
			ratio = 350;
			break;
		case 3:
			ratio = 450;
			break;
		case 4:
			ratio = 550;
			break;
		case 5:
			ratio = 650;
			break;
		}
	return ratio + dex;
}

int AlchemistSkillAttackRatioCalculator::calculate_acid_terror_attack_ratio(int skill_lv, int dex)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 250;
			break;
		case 2:
			ratio = 350;
			break;
		case 3:
			ratio = 450;
			break;
		case 4:
			ratio = 550;
			break;
		case 5:
			ratio = 650;
			break;
		}
	return ratio + dex;
}


int AlchemistSkillAttackRatioCalculator::calculate_demonstration_attack_ratio(int skill_lv, int intelligence)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 225;
			break;
		case 2:
			ratio = 250;
			break;
		case 3:
			ratio = 275;
			break;
		case 4:
			ratio = 300;
			break;
		case 5:
			ratio = 325;
			break;
		}
	return ratio + (intelligence);
}


int AlchemistSkillAttackRatioCalculator::calculate_basilisk_1_attack_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 110;
			break;
		case 2:
			ratio = 220;
			break;
		case 3:
			ratio = 330;
			break;
		case 4:
			ratio = 440;
			break;
		case 5:
			ratio = 550;
			break;
		}
	return ratio;
}


int AlchemistSkillAttackRatioCalculator::calculate_basilisk_2_attack_ratio(int skill_lv)
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

void AlchemistSkillAttackRatioCalculator::calculate_basilisk_1_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_HFLIMOON3, AREA);
}

void AlchemistSkillAttackRatioCalculator::calculate_basilisk_2_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_TRIPLEATTACK, AREA);
}

void AlchemistSkillAttackRatioCalculator::calculate_beholder_1_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_REDLINE, AREA);
}

void AlchemistSkillAttackRatioCalculator::calculate_beholder_2_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_M05, AREA);
}

void AlchemistSkillAttackRatioCalculator::calculate_acid_terror_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_GREENBODY, AREA);
    clif_specialeffect(target, EF_M07, AREA);
}
