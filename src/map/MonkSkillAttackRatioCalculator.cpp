#include "MonkSkillAttackRatioCalculator.h"

/**
 * ATK ratio calculater for Swordsman skills.
 * @param base_lv : player base level
 * @param skill_id : skill id
 * @param skill_lv : skill level
 * @param int_ : player's int
 */
int MonkSkillAttackRatioCalculator::calculate_skill_atk_ratio(struct block_list* src, struct block_list *target, int base_lv, int skill_id, int skill_lv, struct status_data* sstatus, bool revealed_hidden_enemy, map_session_data *sd, status_change *sc)
{
	switch (skill_id) {
		case GS_FULLBUSTER:
			ShowMessage("TRIPLE ARM CANNON\n");
			if(!sc->data[SC_COMBO1] && !sc->data[SC_COMBO2] && !sc->data[SC_COMBO3] &&
				!sc->data[SC_COMBO4] && !sc->data[SC_COMBO5] && !sc->data[SC_COMBO6] &&
				!sc->data[SC_COMBO7] && !sc->data[SC_COMBO8] && !sc->data[SC_COMBO9] && !sc->data[SC_COMBO10]) {
					ShowMessage("SC START\n");
					sc_start(&sd->bl,&sd->bl, SC_COMBO1, 100, 17, 4000);
			}
			return calculate_raging_triple_blow_atk_ratio(skill_lv);
		case GS_PIERCINGSHOT:
		{
			bool is_using_mace = false;
			if (sd && sd->status.weapon == W_MACE || sd->status.weapon == W_2HMACE) {
				is_using_mace = true;
			}
			clif_specialeffect(target, EF_ENERVATION2, AREA); //groomy	
			return calculate_chain_combo_atk_ratio(skill_lv, is_using_mace);
		}
		case NC_BOOSTKNUCKLE:
			clif_specialeffect(target, EF_TINDER_BREAKER, AREA); //tinder
			return 100;
		case NJ_ZENYNAGE:
			clif_specialeffect(target, EF_ENERVATION3, AREA); //ignorance
			return 100;
		
		case MO_FINGEROFFENSIVE:
			return calculate_throw_spirit_sphere_atk_ratio(skill_lv);
        case MO_INVESTIGATE:
            return calculate_occult_impact(skill_lv, status_get_def(target));
		case MO_KI_BLAST:
			return calculate_ki_blast_atk_ratio(skill_lv, sstatus->str);
		case CH_TIGERFIST:
			clif_specialeffect(src, EF_TINDER_BREAKER, AREA); //tinder
			return 100;
		case MO_COMBOFINISH:
			clif_specialeffect(src, EF_ENERVATION3, AREA); //ignorance
			return 100;
		case SR_WINDMILL:
			return calculate_circular_fists_atk_ratio(skill_lv, revealed_hidden_enemy);
		case CH_PALMSTRIKE:
			return calculate_palm_strike_atk_ratio(skill_lv);
		case MO_BALKYOUNG:
			add_falling_fist_special_effects(target);
			return calculate_falling_fist_atk_ratio(skill_lv);
		case SR_KNUCKLEARROW:
			return calculate_dash_punch_atk_ratio(skill_lv);
		case SR_DRAGONCOMBO:
			return calculate_guillotine_fists_atk_ratio(skill_lv);
		default:
			return 0;
			break;
	}
}

void MonkSkillAttackRatioCalculator::add_falling_fist_special_effects(struct block_list *target)
{
	clif_specialeffect(target, EF_COMBOATTACK5, AREA);
	clif_specialeffect(target, EF_DRAGONSMOKE, AREA);
	clif_specialeffect(target, EF_SPINEDBODY, AREA);
	clif_specialeffect(target, EF_KICKEDBODY, AREA);
	clif_specialeffect(target, EF_ENERVATION5, AREA); //unlucky
}


int MonkSkillAttackRatioCalculator::calculate_dash_punch_atk_ratio(int skill_lv)
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


int MonkSkillAttackRatioCalculator::calculate_falling_fist_atk_ratio(int skill_lv)
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

int MonkSkillAttackRatioCalculator::calculate_palm_strike_atk_ratio(int skill_lv)
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


int MonkSkillAttackRatioCalculator::calculate_circular_fists_atk_ratio(int skill_lv, bool revealed_hidden_enemy)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 100;
			break;
		case 2:
			ratio = 150;
			break;
		case 3:
			ratio = 200;
			break;
		case 4:
			ratio = 250;
			break;
		case 5:
			ratio = 300;
			break;
	}
	if (revealed_hidden_enemy) {
		ratio += 200;
	}
	return ratio;
}

int MonkSkillAttackRatioCalculator::calculate_throw_spirit_sphere_atk_ratio(int skill_lv)
{
	return 50;
}

int MonkSkillAttackRatioCalculator::calculate_occult_impact(int skill_lv, defType defence)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 30 + (defence);
			break;
		case 2:
			ratio = 100 + (defence);
			break;
		case 3:
			ratio = 180 + (defence *2);
			break;
		case 4:
			ratio = 240 + (defence *2);
			break;
		case 5:
			ratio = 300 + (defence *3);
			break;
	}
	return ratio;
}


int MonkSkillAttackRatioCalculator::calculate_ki_blast_atk_ratio(int skill_lv, int str)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 100;
			break;
		case 2:
			ratio = 250;
			break;
		case 3:
			ratio = 300;
			break;
		case 4:
			ratio = 450;
			break;
		case 5:
			ratio = 500;
			break;
	}
	return ratio + str/3;
}


int MonkSkillAttackRatioCalculator::calculate_raging_triple_blow_atk_ratio(int skill_lv)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 0;
			break;
		case 2:
			ratio = 75;
			break;
		case 3:
			ratio = 175;
			break;
		case 4:
			ratio = 275;
			break;
		case 5:
			ratio = 375;
			break;
	}
	return ratio;
}


int MonkSkillAttackRatioCalculator::calculate_chain_combo_atk_ratio(int skill_lv, bool is_using_mace)
{
	int ratio = 0;
	switch (skill_lv) {
		case 1:
			ratio = 50;
			break;
		case 2:
			ratio = 200;
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
	if (is_using_mace){
		ratio += 150;
	}
	return ratio;
}

int MonkSkillAttackRatioCalculator::calculate_guillotine_fists_atk_ratio(int skill_lv)
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
