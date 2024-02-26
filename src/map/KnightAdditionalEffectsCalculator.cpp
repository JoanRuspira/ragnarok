#include "KnightAdditionalEffectsCalculator.h"



void KnightAdditionalEffectsCalculator::apply_auto_counter_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_BLEEDING, 10000, skill_lv, 0, 0, 0, 10000, SCSTART_NONE);
}

void KnightAdditionalEffectsCalculator::apply_smite_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 200, SCSTART_NONE);
}

void KnightAdditionalEffectsCalculator::apply_a_hundred_spears_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv, t_tick tick)
{
	int spear_boomerang_chance = 0;
	spear_boomerang_chance = 1 + (rand() % 100);

	if(spear_boomerang_chance <= skill_lv*8){
		clif_skill_damage(src,bl,tick, status_get_amotion(src), 0, -30000, 1, SK_KN_SPEARCANNON, skill_lv, DMG_SINGLE);
	} 
	else {
		clif_skill_damage(src,bl,tick, status_get_amotion(src), 0, -30000, 1, SK_CM_DUMMY_HUNDREDSPEAR, skill_lv, DMG_SINGLE);
	}
}
