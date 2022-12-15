#include "ThiefSkillAtkRatioCalculator.h"

/**
 * ATK ratio calculater for Thief skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int ThiefSkillAtkRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv)
{
	switch (skill_id) {
	case TF_POISON:
		add_envenom_special_effects(src, target);
		return calculate_envenom_atk_ratio(skill_lv);
		break;
	case TF_THROWSTONE:
		add_throw_stone_special_effects(target);
		return calculate_throw_stone_atk_ratio(skill_lv);
		break;
	case TF_SNATCH:
		add_snatch_special_effects(target);
		return calculate_snatch_atk_ratio(skill_lv);
		break;
	case TF_SANDATTACK:
		add_sand_attack_special_effects(target);
		return calculate_sand_attack_atk_ratio(skill_lv);
		break;
	case AS_VENOMKNIFE:
		add_venom_knife_special_effects(src, target);
		return calculate_venom_knife_atk_ratio(skill_lv);
		break;
	case AS_POISONREACT:
		return calculate_poison_react_atk_ratio(skill_lv);
		break;
	default:
		return 0;
		break;
	}
}

void ThiefSkillAtkRatioCalculator::add_venom_knife_special_effects(struct block_list* src, struct block_list *target)
{
	clif_specialeffect(src, EF_BASH, AREA);
	clif_specialeffect(target, EF_HIT5, AREA);
	clif_specialeffect(target, EF_INVENOM, AREA);
}

void ThiefSkillAtkRatioCalculator::add_envenom_special_effects(struct block_list* src, struct block_list *target)
{
	clif_specialeffect(target, EF_POISONATTACK, AREA);
	clif_specialeffect(target, EF_PATTACK, AREA);
}

void ThiefSkillAtkRatioCalculator::add_snatch_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_STEAL, AREA);
	clif_specialeffect(target, EF_STEALCOIN, AREA);
	clif_specialeffect(target, EF_STUNATTACK, AREA);
}

void ThiefSkillAtkRatioCalculator::add_sand_attack_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_SPRINKLESAND, AREA);
	clif_specialeffect(target, EF_DRAGONSMOKE, AREA);
	clif_specialeffect(target, EF_STUNATTACK, AREA);
}

void ThiefSkillAtkRatioCalculator::add_throw_stone_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_STUNATTACK, AREA);
}

int ThiefSkillAtkRatioCalculator::calculate_venom_knife_atk_ratio(int skill_lv)
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


int ThiefSkillAtkRatioCalculator::calculate_poison_react_atk_ratio(int skill_lv)
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

int ThiefSkillAtkRatioCalculator::calculate_envenom_atk_ratio(int skill_lv)
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

int ThiefSkillAtkRatioCalculator::calculate_throw_stone_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
	case 1:
		ratio = -30;
		break;
	case 2:
		ratio = 40;
		break;
	case 3:
		ratio = 110;
		break;
	case 4:
		ratio = 180;
		break;
	case 5:
		ratio = 250;
		break;
	}
	return ratio;
}

int ThiefSkillAtkRatioCalculator::calculate_snatch_atk_ratio(int skill_lv)
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

int ThiefSkillAtkRatioCalculator::calculate_sand_attack_atk_ratio(int skill_lv)
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
