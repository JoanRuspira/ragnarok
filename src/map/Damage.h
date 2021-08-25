#pragma once
#include "../common/cbasetypes.hpp"

/// Battle Damage structure
struct Damage {
	int64 statusAtk, statusAtk2, weaponAtk, weaponAtk2, equipAtk, equipAtk2, masteryAtk, masteryAtk2;
	int64 damage, /// Right hand damage
		damage2; /// Left hand damage
	enum e_damage_type type; /// Check clif_damage for type
	short div_; /// Number of hit
	int amotion,
		dmotion;
	int blewcount; /// Number of knockback
	int flag; /// chk e_battle_flag
	int miscflag;
	enum damage_lv dmg_lv; /// ATK_LUCKY,ATK_FLEE,ATK_DEF
	bool isspdamage; /// Display blue damage numbers in clif_damage
};
