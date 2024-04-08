#include "AlchemistSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int AlchemistSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, struct status_data* tstatus)
{
	switch (skill_id) {
		case SK_AM_BASILISK1:
			calculate_basilisk_1_special_effects(target);
			return calculate_basilisk_1_attack_ratio(skill_lv);
			break;
		case SK_AM_BEHOLDER1:
			calculate_beholder_1_special_effects(target);
			return calculate_basilisk_1_attack_ratio(skill_lv);
			break;
		case SK_CR_BEHOLDER3:
			clif_soundeffectall(target, "abracadabra.wav", 0, AREA);
			clif_specialeffect(target, 1382, AREA);//new_dragonbreath_07_clock
			return calculate_beholder_3_attack_ratio(skill_lv);
			break;
		case SK_CR_BASILISK3:
			clif_soundeffectall(target, "silvervein.wav", 0, AREA);
			clif_specialeffect(target, EF_MIDNIGHT_FRENZY, AREA);
			return calculate_beholder_3_attack_ratio(skill_lv);
			break;
		case SK_AM_BASILISK2:
			return calculate_basilisk_2_attack_ratio(skill_lv);
			break;
		case SK_AM_BEHOLDER2:
			return calculate_basilisk_2_attack_ratio(skill_lv);
			break;
		case SK_AM_FIREDEMONSTRATION:
			return calculate_demonstration_attack_ratio(skill_lv, sstatus->int_);
			break;
		case SK_AM_ACIDTERROR:
			calculate_acid_terror_special_effects(target);
			return calculate_acid_terror_attack_ratio(skill_lv, sstatus->dex);
			break;
		case SK_AM_BOMB:
		 	clif_specialeffect(target, EF_M03, AREA);
			return calculate_bomb_attack_ratio(skill_lv, sstatus->dex);
			break;
		case SK_AM_WILDTHORNS:
			return calculate_wild_thorns_attack_ratio(skill_lv, sstatus->int_);
			break;
		case SK_CR_INCENDIARYBOMB:
			return calculate_incendiary_bomb_atk_ratio(skill_lv, sstatus->int_);
			break;
		case SK_CR_ACIDBOMB:
    		clif_specialeffect(target, EF_GREENBODY, AREA);
    		clif_specialeffect(target, 804, AREA);
    		clif_specialeffect(target, 1406, AREA); //new_cannon_spear_09_clock
			return calculate_acid_bomb_atk_ratio(skill_lv, sstatus->int_, sstatus->dex, tstatus->vit);
			break;
		case SK_CR_GEOGRAFIELD:
			return calculate_geografield_atk_ratio(skill_lv);
			break;
		case SK_CR_MANDRAKERAID_ATK:
			return calculate_mandrake_raid_atk_ratio(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}
int AlchemistSkillAttackRatioCalculator::calculate_mandrake_raid_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 0;
			break;
		case 2:
			ratio = 200;
			break;
		case 3:
			ratio = 400;
			break;
		case 4:
			ratio = 600;
			break;
		case 5:
			ratio = 800;
			break;
	}
	return ratio;
}

int AlchemistSkillAttackRatioCalculator::calculate_geografield_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 0;
			break;
		case 2:
			ratio = 200;
			break;
		case 3:
			ratio = 400;
			break;
		case 4:
			ratio = 600;
			break;
		case 5:
			ratio = 800;
			break;
	}
	return ratio;
}



int AlchemistSkillAttackRatioCalculator::calculate_acid_bomb_atk_ratio(int skill_lv, int intelligence, int dex, int vit)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 350;
			break;
		case 2:
			ratio = 450;
			break;
		case 3:
			ratio = 550;
			break;
		case 4:
			ratio = 650;
			break;
		case 5:
			ratio = 750;
			break;
	}
	return ratio + intelligence + dex;
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

int AlchemistSkillAttackRatioCalculator::calculate_beholder_3_attack_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 400;
			break;
		case 2:
			ratio = 500;
			break;
		case 3:
			ratio = 600;
			break;
		case 4:
			ratio = 700;
			break;
		case 5:
			ratio = 800;
			break;
		}
	return ratio;
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
    clif_specialeffect(target, 1062, AREA);
}

void AlchemistSkillAttackRatioCalculator::calculate_beholder_2_special_effects(struct block_list *target)
{
    clif_specialeffect(target, 1218, AREA);
}

void AlchemistSkillAttackRatioCalculator::calculate_acid_terror_special_effects(struct block_list *target)
{
    clif_specialeffect(target, EF_GREENBODY, AREA);
    clif_specialeffect(target, 1406, AREA); //new_cannon_spear_09_clock
}
