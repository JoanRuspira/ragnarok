#include "CriticalHitCalculator.h"
#include <cstring>
#include <cstdint>
#include "skill.hpp"
#include "status.hpp"
#include "Damage.h"
#include "map.hpp"
#include "homunculus.hpp"
#include "battle.hpp"
#include "pc.hpp"
#include "party.hpp"




#include "battle.hpp"

#include <math.h>
#include <stdlib.h>

#include "../common/cbasetypes.hpp"
#include "../common/ers.hpp"
#include "../common/malloc.hpp"
#include "../common/nullpo.hpp"
#include "../common/random.hpp"
#include "../common/showmsg.hpp"
#include "../common/socket.hpp"
#include "../common/strlib.hpp"
#include "../common/timer.hpp"
#include "../common/utils.hpp"

/*=============================
 * Do we score a critical hit?
 *-----------------------------
 * Credits:
 *	Original coder Skotlex
 *	Initial refactoring by Baalberith
 *	Refined and optimized by helvetica
 */

bool CriticalHitCalculator::is_attack_critical(Damage * wd, block_list * src, block_list * target, int skill_id, int skill_lv, bool first_call)
{
	if (!first_call)
		return (wd->type == DMG_CRITICAL || wd->type == DMG_MULTI_HIT_CRITICAL);


	if (skill_id && !skill_get_nk(skill_id, NK_CRITICAL))
		return false;

	struct status_data *sstatus = status_get_status_data(src);

	if (sstatus->cri)
	{
		struct map_session_data *sd = BL_CAST(BL_PC, src);

		if (wd->type == DMG_MULTI_HIT) {	//Multiple Hit Attack Skills.
			if (pc_checkskill(sd, SK_AS_DOUBLEATTACK) && !skill_get_nk(SK_AS_DOUBLEATTACK, NK_CRITICAL)) //Double Attack
				return false;
		}

		struct status_data *tstatus = status_get_status_data(target);
		struct status_change *sc = status_get_sc(src);
		struct status_change *tsc = status_get_sc(target);
		struct map_session_data *tsd = BL_CAST(BL_PC, target);
		short cri = sstatus->cri;

		if (sd) {
			cri += sd->indexed_bonus.critaddrace[tstatus->race] + sd->indexed_bonus.critaddrace[RC_ALL];
			if (!skill_id && is_skill_using_arrow(src, skill_id)) {
				cri += sd->bonus.arrow_cri;
				cri += sd->bonus.critical_rangeatk;
			}
		}

		if (sc && sc->data[SC_CAMOUFLAGE])
			cri += 100 * min(10, sc->data[SC_CAMOUFLAGE]->val3); //max 100% (1K)

		//The official equation is *2, but that only applies when sd's do critical.
		//Therefore, we use the old value 3 on cases when an sd gets attacked by a mob
		cri -= tstatus->luk * ((!sd && tsd) ? 3 : 2);

		if (tsc && tsc->data[SC_SLEEP])
			cri <<= 1;

		switch (skill_id) {
		case 0:
			if (sc && !sc->data[SC_AUTOCOUNTER])
				break;
			status_change_end(src, SC_AUTOCOUNTER, INVALID_TIMER);
		case SK_KN_COUNTERATTACK:
			if (battle_config.auto_counter_type &&
				(battle_config.auto_counter_type&src->type))
				return true;
			else
				cri <<= 1;
			break;
		case SK_RG_SHADYSLASH:
			cri += 250 + 50 * skill_lv;
			break;
		}
		if (tsd && tsd->bonus.critical_def)
			cri = cri * (100 - tsd->bonus.critical_def) / 100;
		return (rnd() % 1000 < cri);
	}
	return false;
}


bool CriticalHitCalculator::is_skill_using_arrow(struct block_list *src, int skill_id)
{
	if (src != NULL) {
		struct status_data *sstatus = status_get_status_data(src);
		struct map_session_data *sd = BL_CAST(BL_PC, src);

		return ((sd && sd->state.arrow_atk) || (!sd && ((skill_id && skill_get_ammotype(skill_id)) || sstatus->rhw.range > 3)));
	}
	else
		return false;
}
