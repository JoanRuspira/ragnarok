#pragma once
#include "../common/timer.hpp"
#include <cstring>
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"

class AssassinAdditionalEffectsCalculator
{
	public:
		static void apply_meteor_assault_additional_effect(struct block_list* src, struct block_list *bl, int skill_lv);
};
