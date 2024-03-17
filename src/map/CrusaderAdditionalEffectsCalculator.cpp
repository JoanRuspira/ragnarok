#include "CrusaderAdditionalEffectsCalculator.h"




void CrusaderAdditionalEffectsCalculator::apply_holy_cross_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, STATUS_BLIND, 10000, skill_lv, 0, 0, 0, skill_lv * 2000, SCSTART_NONE);
}
