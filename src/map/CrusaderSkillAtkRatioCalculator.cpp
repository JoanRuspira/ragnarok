#include "CrusaderSkillAtkRatioCalculator.h"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"


int CrusaderSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus)
{
	switch (skill_id) {
		case SK_CR_HOLYCROSS:
			return calculate_holy_cross_atk_ratio(skill_lv, sstatus->int_);
			break;
		case SK_CR_GRANDCROSS:
			return calculate_grand_cross_atk_ratio(skill_lv);
			break;
		case SK_PA_RAPIDSMITING:
			return calculate_rapid_smiting_atk_ratio(skill_lv);
			break;
		case SK_PA_SHIELDSLAM:
			add_shield_slam_special_effects(target);
			return calculate_shield_slam_atk_ratio(skill_lv);
			break;
		case SK_PA_GENESISRAY:
			return calculate_genesis_ray_atk_ratio(skill_lv);
			break;
		case SK_PA_GLORIADOMINI:
			return calculate_gloria_domini_atk_ratio(skill_lv);
			break;
		default:
			return 0;
			break;
	}
}

void CrusaderSkillAtkRatioCalculator::add_shield_slam_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_PRESSURE2, AREA);
	clif_specialeffect(target, EF_STUNATTACK, AREA);
}

int CrusaderSkillAtkRatioCalculator::calculate_genesis_ray_atk_ratio(int skill_lv)
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
	return ratio;
}

int CrusaderSkillAtkRatioCalculator::calculate_shield_slam_atk_ratio(int skill_lv)
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

int CrusaderSkillAtkRatioCalculator::calculate_gloria_domini_atk_ratio(int skill_lv)
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

int CrusaderSkillAtkRatioCalculator::calculate_rapid_smiting_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 150;
			break;
		case 2:
			ratio = 250;
			break;
		case 3:
			ratio = 350;
			break;
		case 4:
			ratio = 450;
			break;
		case 5:
			ratio = 550;
			break;
		}
	return ratio;
}

int CrusaderSkillAtkRatioCalculator::calculate_grand_cross_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = -50;
			break;
		case 2:
			ratio = -25;
			break;
		case 3:
			ratio = 0;
			break;
		case 4:
			ratio = 25;
			break;
		case 5:
			ratio = 50;
			break;
		}
	return ratio;
}

int CrusaderSkillAtkRatioCalculator::calculate_holy_cross_atk_ratio(int skill_lv, int intelligence)
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
	return ratio + (intelligence*2);
}

