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

class EquipmentAttackCalculator
{
public:
	static int battle_calc_base_weapon_attack(struct block_list *src, struct status_data *tstatus, struct weapon_atk *wa, struct map_session_data *sd);
	static int battle_calc_equip_attack(struct block_list *src, int skill_id);
	static void battle_add_weapon_damage(struct map_session_data *sd, int64 *damage, int lr_type);
	static int battle_get_weapon_element(struct Damage* wd, struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv, short weapon_position, bool calc_for_damage_only);
	static bool is_attack_left_handed(struct block_list *src, int skill_id);
	static int battle_calc_sizefix(int64 damage, struct map_session_data *sd, unsigned char t_size, unsigned char weapon_type, short flag);
	static bool battle_skill_stacks_masteries_vvs(uint16 skill_id);
};


