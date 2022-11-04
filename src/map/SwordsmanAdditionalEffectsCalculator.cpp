#include "SwordsmanAdditionalEffectsCalculator.h"



void SwordsmanAdditionalEffectsCalculator::apply_bash_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}

void SwordsmanAdditionalEffectsCalculator::apply_traumatic_blow_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_BLEEDING, 10000, skill_lv, 0, 0, 0, 360000, SCSTART_NONE);
}
