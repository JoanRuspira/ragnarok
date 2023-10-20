#include "WizardAdditionalEffectsCalculator.h"




void WizardAdditionalEffectsCalculator::apply_crimson_rock_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STUN, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}

void WizardAdditionalEffectsCalculator::apply_iceberg_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_FREEZE, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}

void WizardAdditionalEffectsCalculator::apply_stalagmite_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_STONE, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}

void WizardAdditionalEffectsCalculator::apply_shadow_bomb_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_CURSE, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}

void WizardAdditionalEffectsCalculator::apply_phantom_spear_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_SLEEP, 10000, skill_lv, 0, 0, 0, skill_lv * 400, SCSTART_NONE);
}

void WizardAdditionalEffectsCalculator::apply_lightning_rod_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv)
{
	status_change_start(src, bl, SC_PARALYSIS, 10000, skill_lv, 0, 0, 0, skill_lv * 600, SCSTART_NONE);
}