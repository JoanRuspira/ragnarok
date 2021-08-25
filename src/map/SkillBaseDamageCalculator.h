#pragma once
#include <cstdint>

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

class SkillBaseDamageCalculator
{
public:
	static void battle_calc_skill_base_damage(struct Damage* wd, struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv);
	static void battle_calc_damage_parts(struct Damage* wd, struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv);
	static int64 battle_calc_base_damage(struct block_list *src, struct status_data *status, struct weapon_atk *wa, struct status_change *sc, unsigned short t_size, int flag);
	static void battle_calc_attack_masteries(struct Damage* wd, struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv);
	static int64 battle_addmastery(struct map_session_data *sd, struct block_list *target, int64 dmg, int type);
	static int battle_calc_status_attack(struct status_data *status, short hand);
	/*static std::bitset<NK_MAX> battle_skill_get_damage_properties(uint16 skill_id, int is_splash);*/
};


