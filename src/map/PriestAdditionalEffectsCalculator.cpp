#include "PriestAdditionalEffectsCalculator.h"




void PriestAdditionalEffectsCalculator::apply_unholy_cross_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, STATUS_CURSE, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
}
