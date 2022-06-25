#pragma once
//
//#include <cstring>
//#include <array>
//#include <math.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <time.h>
//#include <bitset>
//
//#include "map.hpp" // struct block_list
#include "skill.hpp"
#include "status.hpp"
#include "battle.hpp"
#include "unit.hpp"
#include "clif.hpp"
#include "achievement.hpp"
#include "battleground.hpp"
#include "chrif.hpp"
#include "date.hpp"
#include "elemental.hpp"
#include "guild.hpp"
#include "homunculus.hpp"
#include "intif.hpp"
#include "itemdb.hpp"
#include "log.hpp"
#include "map.hpp"
#include "mercenary.hpp"
#include "mob.hpp"
#include "npc.hpp"
#include "party.hpp"
#include "path.hpp"
#include "pc.hpp"
#include "pc_groups.hpp"
#include "script.hpp"

#include "../common/cbasetypes.hpp"
#include "../common/timer.hpp"
#include "../common/ers.hpp"
#include "../common/malloc.hpp"
#include "../common/nullpo.hpp"
#include "../common/random.hpp"
#include "../common/showmsg.hpp"
#include "../common/strlib.hpp"
#include "../common/utilities.hpp"
#include "../common/utils.hpp"
#include "../common/database.hpp"
#include "../common/db.hpp"
#include "../common/mmo.hpp" // MAX_SKILL, struct square



class SkillAdditionalEffects
{
private:

public:
	static int skill_additional_effect(struct block_list* src, struct block_list *bl, uint16 skill_id, uint16 skill_lv, int attack_type, enum damage_lv dmg_lv, t_tick tick);
	
private:
	static bool skill_strip_equip2(struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv);

	static void skill_trigger_status_even_by_blocked_damage(struct block_list* src, struct block_list *bl, struct map_session_data *sd,
		uint16 skill_id, uint16 skill_lv, int &rate, int &attack_type, enum damage_lv dmg_lv);

	static void player_skill_additional_effect(struct block_list* src, struct block_list *bl, struct map_session_data *sd,
		struct status_data *sstatus, struct status_change *sc, struct mob_data *dstmd, int &skill, uint16 skill_id, uint16 skill_lv, int &rate, int &attack_type, t_tick tick);

	static void SkillAdditionalEffects::monster_skill_additional_effect(struct block_list* src, struct block_list *bl,
		struct status_data *sstatus, uint16 skill_id, uint16 skill_lv, int &rate);
};
