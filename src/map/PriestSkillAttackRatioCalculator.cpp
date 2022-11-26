#include "PriestSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int PriestSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case PR_SPIRITUSANCTI:
			add_spiritu_sancti_special_effects(target);
			return calculate_spiritu_sancti_atk_ratio(skill_lv);
			break;
        case AB_JUDEX:
			add_judex_special_effects(target);
			return calculate_judex_atk_ratio(skill_lv);
			break;
		case PR_MAGNUS:
			return calculate_magnus_exorcismus_atk_ratio(skill_lv);
        case PR_UNHOLYCROSS:
			add_unholy_cross_special_effects(target);
			return calculate_unholy_cross_atk_ratio(skill_lv, sstatus->int_);
			break;
		case AB_DUPLELIGHT_MELEE:
			add_duple_liight_melee_special_effects(target);
			return calculate_duple_light_melee_atk_ratio(skill_lv);
			break;
		case AB_DUPLELIGHT_MAGIC:
			add_duple_liight_magic_special_effects(target);
			return calculate_duple_light_magic_atk_ratio(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

void PriestSkillAttackRatioCalculator::add_spiritu_sancti_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_PEONG, AREA);
	clif_specialeffect(target, EF_BANISHING_BUSTER, AREA);
}

void PriestSkillAttackRatioCalculator::add_judex_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_SECRA, AREA);
}

void PriestSkillAttackRatioCalculator::add_unholy_cross_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_KO_JYUMONJIKIRI, AREA);
	clif_specialeffect(target, EF_HOLYCROSS, AREA);
}

void PriestSkillAttackRatioCalculator::add_duple_liight_melee_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_SPELLBREAKER, AREA);
}

void PriestSkillAttackRatioCalculator::add_duple_liight_magic_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_MAGICROD, AREA);
}

int PriestSkillAttackRatioCalculator::calculate_duple_light_melee_atk_ratio(int skill_lv)
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

int PriestSkillAttackRatioCalculator::calculate_duple_light_magic_atk_ratio(int skill_lv)
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


int PriestSkillAttackRatioCalculator::calculate_unholy_cross_atk_ratio(int skill_lv, int intelligence)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 125;
			break;
		case 2:
			ratio = 150;
			break;
		case 3:
			ratio = 175;
			break;
		case 4:
			ratio = 200;
			break;
		case 5:
			ratio = 225;
			break;
		}
	return ratio + (intelligence);
}


int PriestSkillAttackRatioCalculator::calculate_magnus_exorcismus_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = -80;
			break;
		case 2:
			ratio = -70;
			break;
		case 3:
			ratio = -60;
			break;
		case 4:
			ratio = -50;
			break;
		case 5:
			ratio = -40;
			break;
		}
	return ratio;
}

int PriestSkillAttackRatioCalculator::calculate_judex_atk_ratio(int skill_lv)
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

int PriestSkillAttackRatioCalculator::calculate_spiritu_sancti_atk_ratio(int skill_lv)
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
