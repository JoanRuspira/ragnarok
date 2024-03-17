// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef SKILL_HPP
#define SKILL_HPP

#include <array>
#include <bitset>

#include "../common/cbasetypes.hpp"
#include "../common/database.hpp"
#include "../common/db.hpp"
#include "../common/mmo.hpp" // MAX_SKILL, struct square
#include "../common/timer.hpp"

#include "map.hpp" // struct block_list

enum damage_lv : uint8;
enum sc_type : int16;
enum send_target : uint8;
enum e_damage_type : uint8;
enum e_battle_flag : uint16;
enum e_battle_check_target : uint32;
struct map_session_data;
struct homun_data;
struct skill_unit;
struct skill_unit_group;
struct status_change_entry;

#define MAX_SKILL_PRODUCE_DB	83 /// Max Produce DB
#define MAX_PRODUCE_RESOURCE	12 /// Max Produce requirements
#define MAX_SKILL_ARROW_DB		150 /// Max Arrow Creation DB
#define MAX_ARROW_RESULT		5 /// Max Arrow results/created
#define MAX_SKILL_LEVEL 13 /// Max Skill Level (for skill_db storage)
#define MAX_MOBSKILL_LEVEL 100	/// Max monster skill level (on skill usage)
#define MAX_SKILL_CRIMSON_MARKER 3 /// Max Crimson Marker targets (RL_C_MARKER)
#define SKILL_NAME_LENGTH 31 /// Max Skill Name length
#define SKILL_DESC_LENGTH 31 /// Max Skill Desc length

/// Used with tracking the hitcount of Earthquake for skills that can avoid the first attack
#define NPC_EARTHQUAKE_FLAG 0x800

/// Constants to identify a skill's nk value (damage properties)
/// The NK value applies only to non INF_GROUND_SKILL skills
/// when determining skill castend function to invoke.
enum e_skill_nk : uint8 {
	NK_NODAMAGE = 0,
	NK_SPLASH,
	NK_SPLASHSPLIT,
	NK_IGNOREATKCARD,
	NK_IGNOREELEMENT,
	NK_IGNOREDEFENSE,
	NK_IGNOREFLEE,
	NK_IGNOREDEFCARD,
	NK_CRITICAL,
	NK_IGNORELONGCARD,
	NK_MAX,
};

/// Constants to identify the skill's inf value.
enum e_skill_inf : uint16 {
	INF_PASSIVE_SKILL = 0x00, // Used just for skill_db parsing
	INF_ATTACK_SKILL  = 0x01,
	INF_GROUND_SKILL  = 0x02,
	INF_SELF_SKILL    = 0x04, // Skills casted on self where target is automatically chosen
	// 0x08 not assigned
	INF_SUPPORT_SKILL = 0x10,
	INF_TRAP_SKILL    = 0x20,
};

/// Constants to identify the skill's inf2 value.
enum e_skill_inf2 : uint8 {
	INF2_ISQUEST = 0,
	INF2_ISNPC, //NPC skills are those that players can't have in their skill tree.
	INF2_ISWEDDING,
	INF2_ISSPIRIT,
	INF2_ISGUILD,
	INF2_ISSONG,
	INF2_ISENSEMBLE,
	INF2_ISTRAP,
	INF2_TARGETSELF, //Refers to ground placed skills that will target the caster as well (like Grandcross)
	INF2_NOTARGETSELF,
	INF2_PARTYONLY,
	INF2_GUILDONLY,
	INF2_NOTARGETENEMY,
	INF2_ISAUTOSHADOWSPELL, // Skill that available for SK_ST_AUTOSHADOWSPELL
	INF2_ISCHORUS, // Chorus skill
	INF2_IGNOREBGREDUCTION, // Skill that ignore bg reduction
	INF2_IGNOREGVGREDUCTION, // Skill that ignore gvg reduction
	INF2_DISABLENEARNPC, // disable to cast skill if near with NPC [Cydh]
	INF2_TARGETTRAP, // can hit trap-type skill (INF2_ISTRAP) [Cydh]
	INF2_IGNORELANDPROTECTOR, // Skill that can ignore Land Protector
	INF2_ALLOWWHENHIDDEN, // Skill that can be use in hiding
	INF2_ALLOWWHENPERFORMING, // Skill that can be use while in dancing state
	INF2_TARGETEMPERIUM, // Skill that could hit emperium
	INF2_IGNORESTASIS, // Skill that can ignore STATUS_STASIS
	INF2_IGNOREKAGEHUMI, // Skill blocked by kagehumi
	INF2_ALTERRANGEVULTURE, // Skill range affected by SK_AC_VULTURE
	INF2_ALTERRANGESNAKEEYE, // Skill range affected by GS_SNAKEEYE
	INF2_ALTERRANGESHADOWJUMP, // Skill range affected by NJ_SHADOWJUMP
	INF2_ALTERRANGERADIUS, // Skill range affected by SK_WZ_RADIUS
	INF2_ALTERRANGERESEARCHTRAP, // Skill range affected by RA_RESEARCHTRAP
	INF2_IGNOREHOVERING, // Skill that does not affect user that has SC_HOVERING active
	INF2_ALLOWONWARG, // Skill that can be use while riding warg
	INF2_ALLOWONMADO, // Skill that can be used while on Madogear
	INF2_TARGETMANHOLE, // Skill that can be used to target while under SC__MANHOLE effect
	INF2_TARGETHIDDEN, // Skill that affects hidden targets
	INF2_INCREASEDANCEWITHWUGDAMAGE, // Skill that is affected by SC_DANCEWITHWUG
	INF2_IGNOREWUGBITE, // Skill blocked by RA_WUGBITE
	INF2_IGNOREAUTOGUARD , // Skill is not blocked by STATUS_AUTOGUARD (physical-skill only)
	INF2_IGNORECICADA, // Skill is not blocked by SC_UTSUSEMI or SC_BUNSINJYUTSU (physical-skill only)
	INF2_SHOWSCALE, // Skill shows AoE area while casting
	INF2_MAX,
};

/// Constants for skill requirements
enum e_skill_require : uint16 {
	SKILL_REQ_HPCOST = 0x1,
	SKILL_REQ_SPCOST = 0x2,
	SKILL_REQ_HPRATECOST = 0x4,
	SKILL_REQ_SPRATECOST = 0x8,
	SKILL_REQ_MAXHPTRIGGER = 0x10,
	SKILL_REQ_ZENYCOST = 0x20,
	SKILL_REQ_WEAPON = 0x40,
	SKILL_REQ_AMMO = 0x80,
	SKILL_REQ_STATE = 0x100,
	SKILL_REQ_STATUS = 0x200,
	SKILL_REQ_SPIRITSPHERECOST = 0x400,
	SKILL_REQ_ITEMCOST = 0x800,
	SKILL_REQ_EQUIPMENT = 0x1000,
};

/// Constants for skill cast near NPC.
enum e_skill_nonear_npc : uint8 {
	SKILL_NONEAR_WARPPORTAL = 0x1,
	SKILL_NONEAR_SHOP = 0x2,
	SKILL_NONEAR_NPC = 0x4,
	SKILL_NONEAR_TOMB = 0x8,
};

/// Constants for skill cast adjustments.
enum e_skill_cast_flags : uint8 {
	SKILL_CAST_IGNOREDEX = 0x1,
	SKILL_CAST_IGNORESTATUS = 0x2,
	SKILL_CAST_IGNOREITEMBONUS = 0x4,
};

/// Constants for skill copyable options.
enum e_skill_copyable_option : uint8 {
	SKILL_COPY_PLAGIARISM = 0x1,
	SKILL_COPY_REPRODUCE = 0x2,
};

/// Constants for skill unit flags.
enum e_skill_unit_flag : uint8 {
	UF_NONE = 0,
	UF_NOENEMY,	// If 'defunit_not_enemy' is set, the target is changed to 'friend'
	UF_NOREITERATION,	// Spell cannot be stacked
	UF_NOFOOTSET,	// Spell cannot be cast near/on targets
	UF_NOOVERLAP,	// Spell effects do not overlap
	UF_PATHCHECK,	// Only cells with a shootable path will be placed
	UF_NOPC,	// May not target players
	UF_NOMOB,	// May not target mobs
	UF_SKILL,	// May target skills
	UF_DANCE,	// Dance
	UF_ENSEMBLE,	// Duet
	UF_SONG,	// Song
	UF_DUALMODE,	// Spells should trigger both ontimer and onplace/onout/onleft effects.
	UF_NOKNOCKBACK,	// Skill unit cannot be knocked back
	UF_RANGEDSINGLEUNIT,	// hack for ranged layout, only display center
	UF_CRAZYWEEDIMMUNE,	// Immune to Crazy Weed removal
	UF_REMOVEDBYFIRERAIN,	// removed by Fire Rain
	UF_KNOCKBACKGROUP,	// knockback skill unit with its group instead of single unit
	UF_HIDDENTRAP,	// Hidden trap [Cydh]
	UF_MAX,
};

/// Walk intervals at which chase-skills are attempted to be triggered.
/// If you change this, make sure it's an odd value (for icewall block behavior).
#define WALK_SKILL_INTERVAL 5

/// Time that's added to canact delay on castbegin and substracted on castend
/// This is to prevent hackers from sending a skill packet after cast but before a timer triggers castend
const t_tick SECURITY_CASTTIME = 100;

/// Flags passed to skill_attack/skill_area_sub
enum e_skill_display {
	SD_LEVEL     = 0x1000, // skill_attack will send -1 instead of skill level (affects display of some skills)
	SD_ANIMATION = 0x2000, // skill_attack will use '5' instead of the skill's 'type' (this makes skills show an animation). Also being used in skill_attack for splash skill (NK_SPLASH) to check status_check_skilluse
	SD_SPLASH    = 0x4000, // skill_area_sub will count targets in skill_area_temp[2]
	SD_PREAMBLE  = 0x8000, // skill_area_sub will transmit a 'magic' damage packet (-30000 dmg) for the first target selected
};

#define MAX_SKILL_ITEM_REQUIRE	10 /// Maximum required items
#define MAX_SKILL_STATUS_REQUIRE 3 /// Maximum required statuses
#define MAX_SKILL_EQUIP_REQUIRE 10 /// Maximum required equipped item

/// Single skill requirement. !TODO: Cleanup the variable types
struct s_skill_condition {
	int32 hp;								///< HP cost
	int32 mhp;								///< Max HP to trigger
	int32 sp;								/// SP cost
	int32 hp_rate;							/// HP cost (%)
	int32 sp_rate;							/// SP cost (%)
	int32 zeny;								/// Zeny cost
	int32 weapon;							/// Weapon type. Combined bitmask of enum weapon_type (1<<weapon)
	int32 ammo;								/// Ammo type. Combine bitmask of enum ammo_type (1<<ammo)
	int32 ammo_qty;							/// Amount of ammo
	int32 state;							/// State/condition. @see enum e_require_state
	int32 spiritball;						/// Spiritball cost
	t_itemid itemid[MAX_SKILL_ITEM_REQUIRE];	/// Required item
	int32 amount[MAX_SKILL_ITEM_REQUIRE];	/// Amount of item
	std::vector<t_itemid> eqItem;				/// List of equipped item
	std::vector<sc_type> status;			/// List of Status required (SC)
};

/// Skill requirement structure.
struct s_skill_require {
	int32 hp[MAX_SKILL_LEVEL];				///< HP cost
	int32 mhp[MAX_SKILL_LEVEL];				///< Max HP to trigger
	int32 sp[MAX_SKILL_LEVEL];				/// SP cost
	int32 hp_rate[MAX_SKILL_LEVEL];			/// HP cost (%)
	int32 sp_rate[MAX_SKILL_LEVEL];			/// SP cost (%)
	int32 zeny[MAX_SKILL_LEVEL];			/// Zeny cost
	int32 weapon;							/// Weapon type. Combined bitmask of enum weapon_type (1<<weapon)
	int32 ammo;								/// Ammo type. Combine bitmask of enum ammo_type (1<<ammo)
	int32 ammo_qty[MAX_SKILL_LEVEL];		/// Amount of ammo
	int32 state;							/// State/condition. @see enum e_require_state
	int32 spiritball[MAX_SKILL_LEVEL];		/// Spiritball cost
	t_itemid itemid[MAX_SKILL_ITEM_REQUIRE];	/// Required item
	int32 amount[MAX_SKILL_ITEM_REQUIRE];	/// Amount of item
	std::vector<t_itemid> eqItem;				/// List of equipped item
	std::vector<sc_type> status;			/// List of Status required (SC)
};

/// Skill Copyable structure.
struct s_skill_copyable { // [Cydh]
	uint8 option;
	uint16 req_opt;
};

/// Skill Reading Spellbook structure.
struct s_skill_spellbook {
	uint16 nameid, point;
};

/// Database skills
struct s_skill_db {
	uint16 nameid;								///< Skill ID
	char name[SKILL_NAME_LENGTH];				///< AEGIS_Name
	char desc[SKILL_DESC_LENGTH];				///< English Name
	int32 range[MAX_SKILL_LEVEL];				///< Range
	e_damage_type hit;							///< Hit type
	uint16 inf;									///< Inf: 0- passive, 1- enemy, 2- place, 4- self, 16- friend, 32- trap
	e_element element[MAX_SKILL_LEVEL];			///< Element
	std::bitset<NK_MAX> nk;						///< Damage properties
	int32 splash[MAX_SKILL_LEVEL];				///< Splash effect
	uint16 max;									///< Max level
	int32 num[MAX_SKILL_LEVEL];					///< Number of hit
	bool castcancel;							///< Cancel cast when being hit
	uint16 cast_def_rate;						///< Def rate during cast a skill
	e_battle_flag skill_type;					///< Skill type
	int32 blewcount[MAX_SKILL_LEVEL];			///< Blew count
	std::bitset<INF2_MAX> inf2;					///< Skill flags @see enum e_skill_inf2
	int32 maxcount[MAX_SKILL_LEVEL];			///< Max number skill can be casted in same map

	uint8 castnodex;							///< 1 - Not affected by dex, 2 - Not affected by SC, 4 - Not affected by item
	uint8 delaynodex;							///< 1 - Not affected by dex, 2 - Not affected by SC, 4 - Not affected by item

	// skill_nocast_db.txt
	uint32 nocast;								///< Skill cannot be casted at this zone

	uint16 unit_id;								///< Unit ID. @see enum e_skill_unit_id
	uint16 unit_id2;							///< Alternate unit ID. @see enum e_skill_unit_id
	int32 unit_layout_type[MAX_SKILL_LEVEL];	///< Layout type. -1 is special layout, others are square with lenght*width: (val*2+1)^2
	int32 unit_range[MAX_SKILL_LEVEL];			///< Unit cell effect range
	int16 unit_interval;						///< Interval
	int32 unit_target;							///< Unit target.
	std::bitset<UF_MAX> unit_flag;				///< Unit flags.

	int32 cast[MAX_SKILL_LEVEL];				///< Variable casttime
	int32 delay[MAX_SKILL_LEVEL];				///< Global delay (delay before reusing all skills)
	int32 walkdelay[MAX_SKILL_LEVEL];			///< Delay to walk after casting
	int32 upkeep_time[MAX_SKILL_LEVEL];			///< Duration
	int32 upkeep_time2[MAX_SKILL_LEVEL];		///< Duration2
	int32 cooldown[MAX_SKILL_LEVEL];			///< Cooldown (delay before reusing same skill)
#ifdef RENEWAL_CAST
	int32 fixed_cast[MAX_SKILL_LEVEL];			///< If -1 means 20% than 'cast'
#endif

	struct s_skill_require require;				///< Skill requirement

	uint16 unit_nonearnpc_range;				///< Additional range for UF_NONEARNPC or INF2_DISABLENEARNPC [Cydh]
	uint16 unit_nonearnpc_type;					///< Type of NPC [Cydh]

	struct s_skill_damage damage;
	struct s_skill_copyable copyable;

	int32 abra_probability[MAX_SKILL_LEVEL];
	s_skill_spellbook reading_spellbook;
	uint16 improvisedsong_rate;
};

class SkillDatabase : public TypesafeCachedYamlDatabase <uint16, s_skill_db> {
public:
	SkillDatabase() : TypesafeCachedYamlDatabase("SKILL_DB", 1) {

	}

	const std::string getDefaultLocation();
	template<typename T, size_t S> bool parseNode(std::string nodeName, std::string subNodeName, YAML::Node node, T (&arr)[S]);
	uint64 parseBodyNode(const YAML::Node &node);
	void clear();
};

extern SkillDatabase skill_db;

#define MAX_SQUARE_LAYOUT		7	// 15*15 unit placement maximum
#define MAX_SKILL_UNIT_LAYOUT	(48+MAX_SQUARE_LAYOUT)	// 47 special ones + the square ones
#define MAX_SKILL_UNIT_LAYOUT2	17
#define MAX_SKILL_UNIT_COUNT	((MAX_SQUARE_LAYOUT*2+1)*(MAX_SQUARE_LAYOUT*2+1))
struct s_skill_unit_layout {
	int count;
	int dx[MAX_SKILL_UNIT_COUNT];
	int dy[MAX_SKILL_UNIT_COUNT];
};

struct s_skill_nounit_layout {
	int count;
	int dx[MAX_SKILL_UNIT_COUNT];
	int dy[MAX_SKILL_UNIT_COUNT];
};

#define MAX_SKILLTIMERSKILL 50
struct skill_timerskill {
	int timer;
	int src_id;
	int target_id;
	int map;
	short x,y;
	uint16 skill_id,skill_lv;
	int type; // a BF_ type (NOTE: some places use this as general-purpose storage...)
	int flag;
};

#define MAX_SKILLUNITGROUP 25 /// Maximum skill unit group (for same skill each source)
/// Skill unit group
struct skill_unit_group {
	int src_id; /// Caster ID/RID, if player is account_id
	int party_id; /// Party ID
	int guild_id; /// Guild ID
	int bg_id; /// Battleground ID
	int map; /// Map
	int target_flag; /// Holds BCT_* flag for battle_check_target
	int bl_flag; /// Holds BL_* flag for map_foreachin* functions
	t_tick tick; /// Tick when skill unit initialized
	t_tick limit; /// Life time
	int interval; /// Timer interval
	uint16 skill_id, /// Skill ID
		skill_lv; /// Skill level
	int val1, val2, val3; /// Values
	char *valstr; /// String value, used for HT_TALKIEBOX & RG_GRAFFITI
	int unit_id; /// Unit ID (for client effect)
	int group_id; /// Skill Group ID
	int link_group_id; /// Linked group that should be deleted if this one is deleted
	int unit_count, /// Number of unit at this group
		alive_count; /// Number of alive unit
	t_itemid item_id; /// Store item used.
	struct skill_unit *unit; /// Skill Unit
	struct {
		unsigned ammo_consume : 1; // Need to consume ammo
		unsigned song_dance : 2; //0x1 Song/Dance, 0x2 Ensemble
		unsigned guildaura : 1; // Guild Aura
	} state;
};

/// Skill unit
struct skill_unit {
	struct block_list bl;
	struct skill_unit_group *group; /// Skill group reference
	t_tick limit;
	int val1, val2;
	short range;
	unsigned alive : 1;
	unsigned hidden : 1;
};

#define MAX_SKILLUNITGROUPTICKSET 25
struct skill_unit_group_tickset {
	t_tick tick;
	int id;
};

/// Ring of Nibelungen bonuses
enum e_nibelungen_status : uint8 {
	RINGNBL_ASPDRATE = 1,		///< ASPD + 20%
	RINGNBL_ATKRATE,		///< Physical damage + 20%
	RINGNBL_MATKRATE,		///< MATK + 20%
	RINGNBL_HPRATE,			///< Maximum HP + 30%
	RINGNBL_SPRATE,			///< Maximum SP + 30%
	RINGNBL_ALLSTAT,		///< All stats + 15
	RINGNBL_HIT,			///< HIT + 50
	RINGNBL_FLEE,			///< FLEE + 50
	RINGNBL_SPCONSUM,		///< SP consumption - 30%
	RINGNBL_HPREGEN,		///< HP recovery + 100%
	RINGNBL_SPREGEN,		///< SP recovery + 100%
	RINGNBL_MAX,
};

/// Enum for skill_blown
enum e_skill_blown	{
	BLOWN_NONE					= 0x00,
	BLOWN_DONT_SEND_PACKET		= 0x01, // Position update packets must not be sent to the client
	BLOWN_IGNORE_NO_KNOCKBACK	= 0x02, // Ignores players' special_state.no_knockback
	// These flags return 'count' instead of 0 if target is cannot be knocked back
	BLOWN_NO_KNOCKBACK_MAP		= 0x04, // On a WoE/BG map
	BLOWN_MD_KNOCKBACK_IMMUNE	= 0x08, // If target is MD_KNOCKBACK_IMMUNE
	BLOWN_TARGET_NO_KNOCKBACK	= 0x10, // If target has 'special_state.no_knockback'
	BLOWN_TARGET_BASILICA		= 0x20, // If target is in Basilica area
};

/// Create Database item
struct s_skill_produce_db {
	t_itemid nameid; /// Product ID
	unsigned short req_skill; /// Required Skill
	unsigned int req_skill_lv, /// Required Skill Level
		itemlv; /// Item Level
	t_itemid mat_id[MAX_PRODUCE_RESOURCE]; /// Materials needed
	unsigned short mat_amount[MAX_PRODUCE_RESOURCE]; /// Amount of each materials
};
extern struct s_skill_produce_db skill_produce_db[MAX_SKILL_PRODUCE_DB];

/// Creating database arrow
struct s_skill_arrow_db {
	t_itemid nameid; /// Material ID
	t_itemid cre_id[MAX_ARROW_RESULT]; /// Arrow created
	uint16 cre_amount[MAX_ARROW_RESULT]; /// Amount of each arrow created
};
extern struct s_skill_arrow_db skill_arrow_db[MAX_SKILL_ARROW_DB];

/// Abracadabra database
struct s_skill_abra_db {
	uint16 skill_id; /// Skill ID
	std::array<uint16, MAX_SKILL_LEVEL> per; /// Probability summoned
};

class AbraDatabase : public TypesafeYamlDatabase<uint16, s_skill_abra_db> {
public:
	AbraDatabase() : TypesafeYamlDatabase("ABRA_DB", 1) {

	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const YAML::Node& node);
};

struct s_skill_improvise_db {
	uint16 skill_id, per;
};

class ImprovisedSongDatabase : public TypesafeYamlDatabase<uint16, s_skill_improvise_db> {
public:
	ImprovisedSongDatabase() : TypesafeYamlDatabase("IMPROVISED_SONG_DB", 1) {

	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const YAML::Node& node);
};

void do_init_skill(void);
void do_final_skill(void);

/// Cast type
enum e_cast_type { CAST_GROUND, CAST_DAMAGE, CAST_NODAMAGE };
/// Returns the cast type of the skill: ground cast, castend damage, castend no damage
e_cast_type skill_get_casttype(uint16 skill_id); //[Skotlex]
const char*	skill_get_name( uint16 skill_id ); 	// [Skotlex]
const char*	skill_get_desc( uint16 skill_id ); 	// [Skotlex]
int skill_tree_get_max( uint16 skill_id, int b_class );	// Celest

// Accessor to the skills database
uint16 skill_get_index_(uint16 skill_id, bool silent, const char *func, const char *file, int line);
#define skill_get_index(skill_id)  skill_get_index_((skill_id), false, __FUNCTION__, __FILE__, __LINE__) /// Get skill index from skill_id (common usage on source)
int skill_get_type( uint16 skill_id );
e_damage_type skill_get_hit( uint16 skill_id );
int skill_get_inf( uint16 skill_id );
int skill_get_ele( uint16 skill_id , uint16 skill_lv );
int skill_get_max( uint16 skill_id );
int skill_get_range( uint16 skill_id , uint16 skill_lv );
int skill_get_range2(struct block_list *bl, uint16 skill_id, uint16 skill_lv, bool isServer);
int skill_get_splash( uint16 skill_id , uint16 skill_lv );
int skill_get_num( uint16 skill_id ,uint16 skill_lv );
int skill_get_cast( uint16 skill_id ,uint16 skill_lv );
int skill_get_delay( uint16 skill_id ,uint16 skill_lv );
int skill_get_walkdelay( uint16 skill_id ,uint16 skill_lv );
int skill_get_time( uint16 skill_id ,uint16 skill_lv );
int skill_get_time2( uint16 skill_id ,uint16 skill_lv );
int skill_get_castnodex( uint16 skill_id );
int skill_get_castdef( uint16 skill_id );
int skill_get_nocast( uint16 skill_id );
int skill_get_unit_id( uint16 skill_id );
int skill_get_unit_id2( uint16 skill_id );
int skill_get_castcancel( uint16 skill_id );
int skill_get_maxcount( uint16 skill_id ,uint16 skill_lv );
int skill_get_blewcount( uint16 skill_id ,uint16 skill_lv );
int skill_get_cooldown( uint16 skill_id, uint16 skill_lv );
int skill_get_unit_target( uint16 skill_id );
#define skill_get_nk(skill_id, nk) skill_get_nk_(skill_id, { nk })
bool skill_get_nk_(uint16 skill_id, std::vector<e_skill_nk> nk);
#define skill_get_inf2(skill_id, inf2) skill_get_inf2_(skill_id, { inf2 })
bool skill_get_inf2_(uint16 skill_id, std::vector<e_skill_inf2> inf2);
#define skill_get_unit_flag(skill_id, unit) skill_get_unit_flag_(skill_id, { unit })
bool skill_get_unit_flag_(uint16 skill_id, std::vector<e_skill_unit_flag> unit);
int skill_get_unit_range(uint16 skill_id, uint16 skill_lv);
// Accessor for skill requirements
int skill_get_hp( uint16 skill_id ,uint16 skill_lv );
int skill_get_mhp( uint16 skill_id ,uint16 skill_lv );
int skill_get_sp( uint16 skill_id ,uint16 skill_lv );
int skill_get_status_count( uint16 skill_id );
int skill_get_hp_rate( uint16 skill_id, uint16 skill_lv );
int skill_get_sp_rate( uint16 skill_id, uint16 skill_lv );
int skill_get_zeny( uint16 skill_id ,uint16 skill_lv );
int skill_get_weapontype( uint16 skill_id );
int skill_get_ammotype( uint16 skill_id );
int skill_get_ammo_qty( uint16 skill_id, uint16 skill_lv );
int skill_get_state(uint16 skill_id);
int skill_get_status_count( uint16 skill_id );
int skill_get_spiritball( uint16 skill_id, uint16 skill_lv );
unsigned short skill_dummy2skill_id(unsigned short skill_id);

uint16 skill_name2id(const char* name);

uint16 SKILL_MAX_DB(void);

int skill_isammotype(struct map_session_data *sd, unsigned short skill_id);
TIMER_FUNC(skill_castend_id);
TIMER_FUNC(skill_castend_pos);
TIMER_FUNC( skill_keep_using );
int skill_castend_map( struct map_session_data *sd,uint16 skill_id, const char *map);

int skill_cleartimerskill(struct block_list *src);
int skill_addtimerskill(struct block_list *src,t_tick tick,int target,int x,int y,uint16 skill_id,uint16 skill_lv,int type,int flag);

// Results? Added
int skill_additional_effect( struct block_list* src, struct block_list *bl,uint16 skill_id,uint16 skill_lv,int attack_type,enum damage_lv dmg_lv,t_tick tick);
int skill_counter_additional_effect( struct block_list* src, struct block_list *bl,uint16 skill_id,uint16 skill_lv,int attack_type,t_tick tick);
short skill_blown(struct block_list* src, struct block_list* target, char count, int8 dir, enum e_skill_blown flag);
int skill_break_equip(struct block_list *src,struct block_list *bl, unsigned short where, int rate, int flag);
int skill_strip_equip(struct block_list *src,struct block_list *bl, unsigned short where, int rate, int lv, int time);
// Skills unit
struct skill_unit_group *skill_id2group(int group_id);
struct skill_unit_group *skill_unitsetting(struct block_list* src, uint16 skill_id, uint16 skill_lv, int16 x, int16 y, int flag);
struct skill_unit *skill_initunit (struct skill_unit_group *group, int idx, int x, int y, int val1, int val2, bool hidden);
int skill_delunit(struct skill_unit *unit);
struct skill_unit_group *skill_initunitgroup(struct block_list* src, int count, uint16 skill_id, uint16 skill_lv, int unit_id, t_tick limit, int interval);
int skill_delunitgroup_(struct skill_unit_group *group, const char* file, int line, const char* func);
#define skill_delunitgroup(group) skill_delunitgroup_(group,__FILE__,__LINE__,__func__)
void skill_clear_unitgroup(struct block_list *src);
int skill_clear_group(struct block_list *bl, int flag);
void ext_skill_unit_onplace(struct skill_unit *unit, struct block_list *bl, t_tick tick);
int64 skill_unit_ondamaged(struct skill_unit *unit,int64 damage);

// Skill unit visibility [Cydh]
void skill_getareachar_skillunit_visibilty(struct skill_unit *su, enum send_target target);
void skill_getareachar_skillunit_visibilty_single(struct skill_unit *su, struct block_list *bl);

int skill_castfix(struct block_list *bl, uint16 skill_id, uint16 skill_lv);
int skill_castfix_sc(struct block_list *bl, double time, uint8 flag);
#ifdef RENEWAL_CAST
int skill_vfcastfix(struct block_list *bl, double time, uint16 skill_id, uint16 skill_lv);
#endif
int skill_delayfix(struct block_list *bl, uint16 skill_id, uint16 skill_lv);
void skill_toggle_magicpower(struct block_list *bl, uint16 skill_id);

// Skill conditions check and remove [Inkfish]
bool skill_check_condition_castbegin(struct map_session_data *sd, uint16 skill_id, uint16 skill_lv);
bool skill_check_condition_castend(struct map_session_data *sd, uint16 skill_id, uint16 skill_lv);
int skill_check_condition_char_sub (struct block_list *bl, va_list ap);
void skill_consume_requirement(struct map_session_data *sd, uint16 skill_id, uint16 skill_lv, short type);
struct s_skill_condition skill_get_requirement(struct map_session_data *sd, uint16 skill_id, uint16 skill_lv);
int skill_disable_check(struct status_change *sc, uint16 skill_id);
bool skill_pos_maxcount_check(struct block_list *src, int16 x, int16 y, uint16 skill_id, uint16 skill_lv, enum bl_type type, bool display_failure);

int skill_check_pc_partner(struct map_session_data *sd, uint16 skill_id, uint16 *skill_lv, int range, int cast_flag);
int skill_unit_move(struct block_list *bl,t_tick tick,int flag);
void skill_unit_move_unit_group( struct skill_unit_group *group, int16 m,int16 dx,int16 dy);
void skill_unit_move_unit(struct block_list *bl, int dx, int dy);

int skill_sit(struct map_session_data *sd, bool sitting);
void skill_repairweapon(struct map_session_data *sd, int idx);
void skill_identify(struct map_session_data *sd,int idx);
void skill_weaponrefine(struct map_session_data *sd,int idx); // [Celest]
int skill_autospell(struct map_session_data *md,uint16 skill_id);

int skill_calc_heal(struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv, bool heal);

bool skill_check_cloaking(struct block_list *bl, struct status_change_entry *sce);

// Abnormal status
void skill_enchant_elemental_end(struct block_list *bl, int type);
bool skill_isNotOk(uint16 skill_id, struct map_session_data *sd);
bool skill_isNotOk_hom(struct homun_data *hd, uint16 skill_id, uint16 skill_lv);
bool skill_isNotOk_mercenary(uint16 skill_id, struct mercenary_data *md);

bool skill_isNotOk_npcRange(struct block_list *src, uint16 skill_id, uint16 skill_lv, int pos_x, int pos_y);

// Item creation
short skill_can_produce_mix( struct map_session_data *sd, t_itemid nameid, int trigger, int qty, short req_skill = 0);
bool skill_produce_mix( struct map_session_data *sd, uint16 skill_id, t_itemid nameid, int slot1, int slot2, int slot3, int qty, short produce_idx );

bool skill_arrow_create( struct map_session_data *sd, t_itemid nameid);

// skills for the mob
int skill_castend_nodamage_id( struct block_list *src, struct block_list *bl,uint16 skill_id,uint16 skill_lv,t_tick tick,int flag );
int skill_castend_damage_id( struct block_list* src, struct block_list *bl,uint16 skill_id,uint16 skill_lv,t_tick tick,int flag, bool is_automatic=false);
int skill_castend_pos2( struct block_list *src, int x,int y,uint16 skill_id,uint16 skill_lv,t_tick tick,int flag);

int skill_blockpc_start(struct map_session_data*, int, t_tick);
int skill_blockpc_get(struct map_session_data *sd, int skillid);
int skill_blockpc_clear(struct map_session_data *sd);
TIMER_FUNC(skill_blockpc_end);
int skill_blockhomun_start (struct homun_data*,uint16 skill_id,int);
int skill_blockmerc_start (struct mercenary_data*,uint16 skill_id,int);


// // (Epoque:) To-do: replace this macro with some sort of skill tree check (rather than hard-coded skill names)
// #define skill_ischangesex(id) ( \
// 	((id) >= BD_ADAPTATION     && (id) <= DC_SERVICEFORYOU) || ((id) >= SK_CL_ARROWVULCAN && (id) <= SK_CL_MARIONETTECONTROL) || \
// 	((id) >= CG_LONGINGFREEDOM && (id) <= CG_TAROTCARD)     || ((id) >= WA_SWING_DANCE && (id) <= WM_UNLIMITED_HUMMING_VOICE))

// Skill action, (return dmg,heal)
int64 skill_attack( int attack_type, struct block_list* src, struct block_list *dsrc,struct block_list *bl,uint16 skill_id,uint16 skill_lv,t_tick tick,int flag );

void skill_reload(void);

/// List of State Requirements
enum e_require_state : uint8 {
	ST_NONE,
	ST_HIDDEN,
	ST_RIDING,
	ST_FALCON,
	ST_CART,
	ST_SHIELD,
	ST_RECOVER_WEIGHT_RATE,
	ST_MOVE_ENABLE,
	ST_WATER,
	ST_RIDINGDRAGON,
	ST_WUG,
	ST_RIDINGWUG,
	ST_MADO,
	ST_ELEMENTALSPIRIT,
	ST_ELEMENTALSPIRIT2,
	ST_PECO,
	ST_SUNSTANCE,
	ST_MOONSTANCE,
	ST_STARSTANCE,
	ST_UNIVERSESTANCE
};

/// List of Skills
enum e_skill {
/// Novice
	SK_NV_BASIC = 1,
	SK_NV_EQSWITCH=5067,
	SK_NV_TRICKDEAD=143,
	SK_NV_FIRSTAID=142,
/// Swordman
	SK_SM_TWOHAND=3,
    SK_SM_TRAUMATIC_STRIKE=398,
    SK_SM_PROVOKE=6,
    SK_SM_AUTOBERSERK=146,
    SK_SM_SPEARMASTERY=55,
    SK_SM_BASH=5,
    SK_SM_SPEARSTANCE=813,
    SK_SM_SPEARSTAB=58,
    SK_SM_SWORD=2,
    SK_SM_LIGHTNING_STRIKE=399,
    SK_SM_SPIRITGROWTH=819,
    SK_SM_MAGNUM=7,
    SK_SM_RECOVERY=4,
    SK_SM_INNER_STRENGTH=48,
    SK_SM_SRECOVERY=9,
    SK_SM_MEDITATE=363,
/// Magician
	SK_MG_EARTHBOLT=839,
    SK_MG_LIGHTNINGBOLT=20,
    SK_MG_COLDBOLT=14,
    SK_MG_FIREBOLT=19,
    SK_MG_SRECOVERY=837,
    SK_MG_CASTCANCEL=275,
    SK_MG_ENERGYCOAT=157,
    SK_MG_READING_SB=2231,
	SK_MG_READING_SB_READING=5078,
    SK_MG_VOIDEXPANSION=2202,
    SK_MG_DARKSTRIKE=340,
    SK_MG_SOULSTRIKE=13,
    SK_MG_CORRUPT=800,
    SK_MG_STONECURSE=16,
    SK_MG_QUAGMIRE=92,
    SK_MG_FROSTDIVER=15,
    SK_MG_ICEWALL=87,
/// Archer
	SK_AC_OWL=43,
    SK_AC_MAKINGARROW=147,
    SK_AC_ARROWRAIN=1009,
    SK_AC_SPIRITUALSTRAFE=499,
    SK_AC_BOW=2430,
	SK_AC_CHARGEDARROW=2017,
    SK_AC_ARROWSHOWER=47,
    SK_AC_DOUBLESTRAFE=46,
    SK_AC_VULTURE=44,
    SK_AC_HAWK=826,
    SK_AC_AGI=827,
    SK_AC_IMPROVECONCENTRATION=45,
    SK_AC_PARALIZINGDART=807,
    SK_AC_TRANQUILIZINGDART=831,
/// Acolyte
    SK_AL_HOLYGHOST=156,
    SK_AL_SACREDWAVE=11,
    SK_AL_SIGNUMCRUCIS=32,
    SK_AL_JGHEAL=895,
    SK_AL_HEAL=28,
    SK_AL_RENEW=2050,
    SK_AL_RUWACH=24,
    SK_AL_ANGELUS=33,
    SK_AL_INCREASEAGI=29,
    SK_AL_BLESSING=34,
    SK_AL_MACEMASTERY=65,
    SK_AL_MACEQUICKEN=820,
/// Merchant
    SK_MC_CARTQUAKE=809,
    SK_MC_CARTCYCLONE=810,
    SK_MC_CARTBRUME=811,
    SK_MC_CARTFIREWORKS=804,
    SK_MC_AXEMASTERY=226,
    SK_MC_MAMMONITE=42,
    SK_MC_LOUD=155,
    SK_MC_UPROAR=818,
    SK_MC_GREED=1013,
    SK_MC_OVERCHARGE=38,
    SK_MC_COINFLIP=893,
    SK_MC_VENDING=41,
/// Thief
    SK_TF_STEAL=50,
    SK_TF_SNATCH=802,
    SK_TF_SANDATTACK=803,
    SK_TF_MUG=210,
    SK_TF_DAGGER=801,
    SK_TF_POISONSLASH=52,
    SK_TF_POISONREACT=139,
    SK_TF_VENOMKNIFE=1004,
    SK_TF_HIDING=51,
    SK_TF_BACKSLIDING=150,
    SK_TF_THROWSTONE=152,
    SK_TF_SWORD=856,
/// Knight
    SK_KN_THQUICKEN=817,
    SK_KN_ADRENALINERUSH=835,
    SK_KN_FURY=876,
    SK_KN_RECKONING=368,
    SK_KN_SQUICKEN=816,
    SK_KN_SPEARCANNON=59,
    SK_KN_PIERCE=56,
    SK_KN_BRANDISHSPEAR=57,
    SK_KN_OHQUICKEN=815,
    SK_KN_WINDCUTTER=2005,
    SK_KN_ENCHANTBLADE=2001,
    SK_KN_SONICWAVE=2002,
    SK_KN_VIGOR=2016,
    SK_KN_STONESKIN=2015,
    SK_KN_COUNTERATTACK=61,
    SK_KN_BOWLINGBASH=62,
/// Crusader
    SK_CR_SWORNPROTECTOR=255,
    SK_CR_SWORDSTOPLOWSHARES=832,
    SK_CR_BINDINGOATH=833,
    SK_CR_DIVINELIGHT=834,
    SK_CR_OHQUICKEN=862,
    SK_CR_AUTOGUARD=249,
    SK_CR_REFLECTSHIELD=252,
    SK_CR_VANGUARDFORCE=2318,
    SK_CR_SMITE=250,
    SK_CR_SHIELDBOOMERANG=251,
    SK_CR_HOLYCROSS=253,
    SK_CR_GRANDCROSS=254,
/// Assassin
    SK_AS_KATAR=134,
    SK_AS_SONICBLOW=136,
    SK_AS_ROLLINGCUTTER=2036,
    SK_AS_CROSSRIPPERSLASHER=2037,
    SK_AS_CLOAKING=135,
    SK_AS_GRIMTOOTH=137,
    SK_AS_STEALTH=389,
    SK_AS_PHANTOMMENACE=214,
    SK_AS_ENCHANTPOISON=138,
    SK_AS_POISONOUSCLOUD=2450,
    SK_AS_VENOMIMPRESS=2021,
    SK_AS_VENOMSPLASHER=141,
    SK_AS_SHURIKEN=523,
    SK_AS_KUNAI=524,
    SK_AS_DUALWIELDING=133,
    SK_AS_DOUBLEATTACK=48,
	SK_AS_DUMMY_SONICBLOW=868,
/// Rogue
    SK_RG_DAGGERQUICKEN=814,
    SK_RG_SHADYSLASH=530,
    SK_RG_BACKSTAB=212,
    SK_RG_HACKANDSLASH=2029,
    SK_RG_VULTURE=836,
    SK_RG_QUICKSHOT=2236,
    SK_RG_TRIANGLESHOT=2288,
    SK_RG_ARROWSTORM=2233,
    SK_RG_CLOSECONFINE=1005,
    SK_RG_STRIPACCESSORY=871,
    SK_RG_PLAGIARISM=872,
    SK_RG_AMBIDEXTERITY=873,
/// Priest
    SK_PR_SPIRITUSANCTI=806,
    SK_PR_JUDEX=881,
    SK_PR_MAGNUSEXORCISMUS=79,
    SK_PR_UNHOLYCROSS=805,
    SK_PR_LEXAETERNA=5072,
    SK_PR_ASPERSIO=68,
    SK_PR_COLUSEOHEAL=2043,
    SK_PR_SANCTUARIO=70,
    SK_PR_SUBLIMITASHEAL=2051,
    SK_PR_IMPOSITIO=66,
    SK_PR_DUPLELUX=2054,
    SK_PR_KYRIE=73,
    SK_PR_MAGNIFICAT=74,
    SK_PR_SUFFRAGIUM=67,
    SK_PR_RESURRECTIO=54,
	SK_PR_DUPLELUX_MELEE=2055,
	SK_PR_DUPLELUX_MAGIC=2056,
/// Monk
    SK_MO_KNUCKLEQUICKEN=855,
    SK_MO_FALINGFIST=1016,
    SK_MO_CIRCULARFISTS=2337,
    SK_MO_PALMSTRIKE=370,
    SK_MO_SUMMONSPIRITSPHERE=401,
	SK_MO_TRHOWSPIRITSPHERE=267,
	SK_MO_OCCULTIMPACT=266,
	SK_MO_KIBLAST=874,
    SK_MO_TRIPLEARMCANNON=519,
    SK_MO_TIGERFIST=514,
    SK_MO_ABSORBSPIRITSPHERE=262,
    SK_MO_BODYRELOCATION=264,
	SK_MO_DECAGI=30,
/// Hunter
    SK_HT_CAMOUFLAGE=2247,
    SK_HT_ELEMENTALTOTEM=2282,
    SK_HT_MAGICTOMAHAWK=337,
    SK_HT_MYSTICTOTEM=824,
    SK_HT_BOWQUICKEN=823,
    SK_HT_WARGTRAINING=2458,
    SK_HT_SLASH=857,
    SK_HT_FALCONRY=2459,
    SK_HT_BLITZBEAT=858,
    SK_HT_TRACKINGBREEZE=2235,
    SK_HT_CYCLONICCHARGE=825,
    SK_HT_WINDRACER=383,
    SK_HT_LIVINGTORNADO=513,
	SK_WG_SLASH=2243,
	SK_FC_BLITZBEAT=129,
/// Performer
    SK_BA_WINDMILLPOEM=2381,
    SK_BA_SHOWMANSHIP=315,
    SK_BA_ACOUSTICRYTHM=307,
    SK_BA_IMPRESSIVERIFF=320,
    SK_BA_MAGICSTRINGS=321,
    SK_BA_MELODYSTRIKE=316,
    SK_BA_GREATECHO=2426,
    SK_BA_METALLICSOUND=2413,
    SK_BA_REVERBERATION=2414,
    SK_BA_ECHONOCTURNAE=2382,
    SK_BA_TWILIGHTCHORD=2423,
    SK_BA_MOONLIGHTSONATA=312,
    SK_BA_ALLNIGHTLONG=898,
/// Blacksmith
    SK_BS_CARTBOOST=387,
    SK_BS_CARTCANNON=2477,
    SK_BS_CARTSHRAPNEL=812,
    SK_BS_CROWDCONTROLSHOT=520,
    SK_BS_WEAPONRYEXPERTISE=111,
    SK_BS_WEAPONSHARPENING=112,
    SK_BS_MAXIMIZEPOWER=113,
    SK_BS_POWERTHRUST=114,
    SK_BS_AXEQUICKEN=808,
    SK_BS_AXEBOOMERANG=2278,
    SK_BS_AXETORNADO=2280,
    SK_BS_HAMMERFALL=110,
    SK_BS_ENDURE=8,
    SK_BS_SKINTEMPERING=109,
    SK_BS_REPAIRWEAPON=108,
    BS_AXE=101,
/// Alchemist
    SK_AM_BIOETHICS=238,
    SK_AM_HOMUNCULUSACTIONI=847,
	SK_AM_HATCHHOMUNCULUS=2457,
    SK_AM_REGENETATION=850,
    SK_AM_HOMUNCULUSACTIONII=851,
    SK_AM_PHARMACY=228,
	SK_AM_FIREDEMONSTRATION=229,
	SK_AM_ACIDTERROR=230,
    SK_AM_BOMB=490,
    SK_AM_PLANTCULTIVATION=491,
    SK_AM_WILDTHORNS=2482,
    SK_AM_AIDPOTION=231,
    SK_AM_AIDCONDENSEDPOTION=854,
	SK_AM_BASILISK1=518,
	SK_AM_BEHOLDER1=849,
	SK_AM_BEHOLDER2=852,
	SK_AM_PETROLOGY=900,
	SK_AM_PYROTECHNIA=901,
	SK_AM_HEALPULSE=902,
	SK_AM_WARMWIND=8001,
	SK_AM_BASILISK2=904,
/// Wizard
    SK_WZ_EARTHSTRAIN=2446,
    SK_WZ_LORDOFVERMILION=85,
    SK_WZ_STORMGUST=89,
    SK_WZ_METEORSTORM=83,
    SK_WZ_STALAGMITE=90,
    SK_WZ_THUNDERSTORM=884,
    SK_WZ_ICEBERG=838,
    SK_WZ_CRIMSONROCK=2211,
    SK_WZ_ICONOFSIN=844,
    SK_WZ_ILLUSIONARYBLADES=469,
    SK_WZ_EXTREMEVACUUM=845,
    SK_WZ_REALITYBREAKER=846,
    SK_WZ_RADIUS=2208,
    SK_WZ_FORESIGHT=403,
    SK_WZ_MAGICCRASHER=365,
    SK_WZ_PSYCHICWAVE=2449,
/// Sage
    SK_SA_ELEMENTALCONTROL=905,
	SK_SA_SUMMONELEMENTAL=2460,
    SK_SA_ELEMENTALACTION1=2461,
    SK_SA_ELEMENTALTRANSFUSION=2464,
    SK_SA_ELEMENTALACTION2=829,
    SK_SA_SILENCE=76,
    SK_SA_DISPELL=289,
    SK_SA_MINDGAMES=402,
    SK_SA_WHITEIMPRISON=2201,
    SK_SA_BOOKQUICKEN=830,
    SK_SA_AUTOSPELL=279,
    SK_SA_SOULBURN=375,
    SK_SA_DRAINLIFE=2210,
	EL_TORNADO_JG=840,
	SK_SA_FIREBOMB=843,
	SK_SA_WATERBLAST=842,
	SK_SA_ROCKCRUSHER=841,
	SK_SA_WINDSLASH=8434,
	SK_SA_EARTHSPIKE=8439,
	SK_SA_FIREBALL=907,
	SK_SA_ICICLE=906,
/// Commander
    SK_CM_TWOHAND=863,
    SK_CM_FRENZY=359,
    SK_CM_CLASHINGSPIRAL=397,
    SK_CM_AURABLADE=355,
    SK_CM_SPEAR_DYNAMO=357,
    SK_CM_HUNDREDSPEAR=2004,
    SK_CM_PARRY=356,
    SK_CM_MILLENIUMSHIELDS=5013,
    SK_CM_DRAGONBREATH=5004,
    SK_CM_IGNITIONBREAK=2006,
	SK_CM_DUMMY_HUNDREDSPEAR=867,
/// Paladin
	SK_PA_GOSPEL=369,
    SK_PA_KINGSGRACE=2322,
    SK_PA_GLORIADOMINI=367,
    SK_PA_GENESISRAY=2321,
    SK_PA_FORTIFY=861,
    SK_PA_DEFENDINGAURA=257,
    SK_PA_SHIELDSLAM=860,
    SK_PA_RAPIDSMITING=480,
/// Executioner
    SK_EX_DARKCLAW=5001,
    SK_EX_DUMMY_CROSSIMPACT=869,
    SK_EX_SOULDESTROYER=379,
    SK_EX_METEORASSAULT=406,
    SK_EX_HALLUCINATIONWALK=2035,
    SK_EX_INVISIBILITY=2290,
    SK_EX_DANCINGBLADES=870,
    SK_EX_SONICACCELERATION=1003,
    SK_EX_POISONBUSTER=2448,
    SK_EX_ENCHANTDEADLYPOISON=378,
	SK_EX_CROSSIMPACT=2022,
/// Stalker
    SK_ST_KILLERINSTIINCT=390,
    SK_ST_FATALMENACE=2258,
    SK_ST_STALK=2287,
    SK_ST_FULLSTRIP=476,
    SK_ST_REPRODUCE=2285,
    SK_ST_AUTOSHADOWSPELL=2286,
    SK_ST_UNLIMIT=5002,
    SK_ST_SHARPSHOOTING=382,
	SK_ST_STRIPWEAPON=215,
	SK_ST_STRIPSHIELD=216,
	SK_ST_STRIPARMOR=217,
	SK_ST_STRIPHELM=218,
/// Bishop
    SK_BI_LAUDATEDOMINIUM=877,
    SK_BI_SENTENTIA=2040,
    SK_BI_PENITENTIA=878,
    SK_BI_DIABOLICRUCIATUS=879,
    SK_BI_OFFERTORIUM=5011,
    SK_BI_EPICLESIS=2044,
    SK_BI_EXPIATIO=2053,
    SK_BI_INLUCEMBATALIA=880,
    SK_BI_BENEDICTIO=69,
    SK_BI_ASSUMPTIO=361,
/// Shaolin
    SK_SH_ULTRAINSTINCT=270,
    SK_SH_GUILLOTINEFISTS=2326,
    SK_SH_GATEOFHELL=526,
    SK_SH_DRAGONDARKNESSFLAME=2256,
    SK_SH_ZEN=875,
    SK_SH_ASURASTRIKE=271,
    SK_SH_GRAPPLE=269,
    SK_SH_MENTALSTRENGTH=268,
/// Ranger
    SK_RA_SPIRITANIMAL=822,
    SK_RA_ULLREAGLETOTEM=899,
	SK_RA_ULLREAGLETOTEM_ATK=2491,
    SK_RA_TYHPOONFLOW=2234,
    SK_RA_ZEPHYRSNIPING=512,
	SK_RA_FALCONEYES=380,
	SK_RA_FALCONASSAULT=381,
/// Rhapsodist
    SK_CL_ARROWVULCAN=394,
	SK_CL_DUMMY_ARROWVULCAN=866,
    SK_CL_SEVERERAINSTORM=2418,
	SK_CL_SEVERERAINSTORM_MELEE=2516,
    SK_CL_MARIONETTECONTROL=396,
    SK_CL_BATTLEDRUMS=309,
    SK_CL_DEEPSLEEPLULLABY=2422,
    SK_CL_METALLICFURY=864,
    SK_CL_ALLSTAR=865,
    SK_CL_PAGANPARTY=313,
/// Sorcerer
    SK_SO_SUMMONELEMENTALSPHERES=2222,
    SK_SO_TETRAVORTEX=2217,
    SK_SO_HINDSIGHT=2206,
    SK_SO_MYSTICALAMPLIFICATION=366,
    SK_SO_PHANTOMSPEAR=883,
    SK_SO_SHADOWBOMB=882,
    SK_SO_DIAMONDDUST=887,
    SK_SO_ASTRALSTRIKE=888,
    SK_SO_VOIDMAGEPRODIGY=5012,
    SK_SO_DOOM=885,
	SK_SO_DOOM_GHOST=886,
	SK_SO_TETRAVORTEX_NEUTRAL=889,
	SK_SO_TETRAVORTEX_WATER=2219,
	SK_SO_TETRAVORTEX_WIND=2220,
	SK_SO_TETRAVORTEX_GROUND=2221,
	SK_SO_TETRAVORTEX_FIRE=895,
/// Professor
    SK_PF_SPELLFIST=2445,
    SK_PF_MAGICSQUARED=482,
    SK_PF_LANDPROTECTOR=288,
    SK_PF_STASIS=894,
    SK_PF_INDULGE=373,
    SK_PF_SOULEXHALE=374,
/// Mastersmith
    SK_MS_TRIGGERHAPPYCART=515,
    SK_MS_CARTTERMINATION=485,
    SK_MS_ARMORREINFORCEMENT=891,
    SK_MS_NEUTRALBARRIER=2273,
    SK_MS_MELTDOWN=384,
    SK_MS_MAXPOWERTHRUST=486,
    SK_MS_WEAPONREFINE=477,
    SK_MS_MASTERCRAFT=892,
    SK_MS_HAMMERDOWNPROTOCOL=890,
    SK_MS_POWERSWING=2279,
/// Creator
    SK_CR_HOMUNCULUSRESEARCH=903,
    SK_CR_GEOGRAFIELD=896,
	SK_CR_GEOGRAFIELD_ATK=897,
    SK_CR_MANDRAKERAID=2483,
	SK_CR_MANDRAKERAID_ATK=2484,
    SK_CR_INCENDIARYBOMB=2485,
    SK_CR_ACIDBOMB=2481,
    SK_CR_FULLCHEMICALPROTECTION=479,
    SK_CR_POTIONPITCHER=478,
	SK_CR_CP_WEAPON=234,
	SK_CR_CP_SHIELD=235,
	SK_CR_CP_ARMOR=236,
	SK_CR_CP_HELM=237,
};

/// The client view ids for land skills.
enum e_skill_unit_id : uint16 {
	UNT_SAFETYWALL = 0x7e,
	UNT_FIREWALL,
	UNT_WARP_WAITING,
	UNT_WARP_ACTIVE,
	UNT_BENEDICTIO, //TODO
	UNT_SANCTUARY,
	UNT_MAGNUS,
	UNT_PNEUMA,
	UNT_DUMMYSKILL, //These show no effect on the client
	UNT_FIREPILLAR_WAITING,
	UNT_FIREPILLAR_ACTIVE,
	UNT_HIDDEN_TRAP, //TODO
	UNT_TRAP, //TODO
	UNT_HIDDEN_WARP_NPC, //TODO
	UNT_USED_TRAPS,
	UNT_ICEWALL,
	UNT_QUAGMIRE,
	UNT_BLASTMINE,
	UNT_SKIDTRAP,
	UNT_ANKLESNARE,
	UNT_VENOMDUST,
	UNT_LANDMINE,
	UNT_SHOCKWAVE,
	UNT_SANDMAN,
	UNT_FLASHER,
	UNT_FREEZINGTRAP,
	UNT_CLAYMORETRAP,
	UNT_TALKIEBOX,
	UNT_VOLCANO,
	UNT_DELUGE,
	UNT_VIOLENTGALE,
	UNT_LANDPROTECTOR,
	UNT_LULLABY,
	UNT_RICHMANKIM,
	UNT_ETERNALCHAOS,
	UNT_DRUMBATTLEFIELD,
	UNT_RINGNIBELUNGEN,
	UNT_ROKISWEIL,
	UNT_INTOABYSS,
	UNT_SIEGFRIED,
	UNT_DISSONANCE,
	UNT_WHISTLE,
	UNT_ASSASSINCROSS,
	UNT_POEMBRAGI,
	UNT_APPLEIDUN,
	UNT_UGLYDANCE,
	UNT_HUMMING,
	UNT_DONTFORGETME,
	UNT_FORTUNEKISS,
	UNT_SERVICEFORYOU,
	UNT_GRAFFITI,
	UNT_DEMONSTRATION,
	UNT_CALLFAMILY,
	UNT_GOSPEL,
	UNT_BASILICA,
	UNT_MOONLIT,
	UNT_FOGWALL,
	UNT_SPIDERWEB,
	UNT_GRAVITATION,
	UNT_HERMODE,
	UNT_KAENSIN, //TODO
	UNT_SUITON,
	UNT_TATAMIGAESHI,
	UNT_KAEN,
	UNT_GROUNDDRIFT_WIND,
	UNT_GROUNDDRIFT_DARK,
	UNT_GROUNDDRIFT_POISON,
	UNT_GROUNDDRIFT_WATER,
	UNT_GROUNDDRIFT_FIRE,
	UNT_DEATHWAVE, //TODO
	UNT_WATERATTACK, //TODO
	UNT_WINDATTACK, //TODO
	UNT_EARTHQUAKE,
	UNT_EVILLAND,
	UNT_DARK_RUNNER, //TODO
	UNT_DARK_TRANSFER, //TODO
	UNT_EPICLESIS,
	UNT_EARTHSTRAIN,
	UNT_MANHOLE,
	UNT_DIMENSIONDOOR,
	UNT_CHAOSPANIC,
	UNT_MAELSTROM,
	UNT_BLOODYLUST,
	UNT_FEINTBOMB,
	UNT_MAGENTATRAP,
	UNT_COBALTTRAP,
	UNT_MAIZETRAP,
	UNT_VERDURETRAP,
	UNT_FIRINGTRAP,
	UNT_ICEBOUNDTRAP,
	UNT_ELECTRICSHOCKER,
	UNT_CLUSTERBOMB,
	UNT_REVERBERATION,
	UNT_SEVERE_RAINSTORM,
	UNT_FIREWALK,
	UNT_ELECTRICWALK,
	UNT_NETHERWORLD,
	UNT_PSYCHIC_WAVE,
	UNT_CLOUD_KILL,
	UNT_POISONSMOKE,
	UNT_NEUTRALBARRIER,
	UNT_STEALTHFIELD,
	UNT_WARMER,
	UNT_THORNS_TRAP,
	UNT_WALLOFTHORN,
	UNT_DEMONIC_FIRE,
	UNT_FIRE_EXPANSION_SMOKE_POWDER,
	UNT_FIRE_EXPANSION_TEAR_GAS,
	UNT_HELLS_PLANT, // No longer a unit skill
	UNT_VACUUM_EXTREME,
	UNT_BANDING,
	UNT_FIRE_MANTLE,
	UNT_WATER_BARRIER,
	UNT_ZEPHYR,
	UNT_POWER_OF_GAIA,
	UNT_FIRE_INSIGNIA,
	UNT_WATER_INSIGNIA,
	UNT_WIND_INSIGNIA,
	UNT_EARTH_INSIGNIA,
	UNT_POISON_MIST,
	UNT_LAVA_SLIDE,
	UNT_VOLCANIC_ASH,
	UNT_ZENKAI_WATER,
	UNT_ZENKAI_LAND,
	UNT_ZENKAI_FIRE,
	UNT_ZENKAI_WIND,
	UNT_MAKIBISHI,
	UNT_VENOMFOG,
	UNT_ICEMINE,
	UNT_FLAMECROSS,
	UNT_HELLBURNING,
	UNT_MAGMA_ERUPTION,
	UNT_KINGS_GRACE,

	UNT_GLITTERING_GREED,
	UNT_B_TRAP,
	UNT_FIRE_RAIN,

	UNT_CATNIPPOWDER,
	UNT_NYANGGRASS,

	UNT_CREATINGSTAR,

	/**
	 * Guild Auras
	 **/
	UNT_GD_LEADERSHIP = 0xc1,
	UNT_GD_GLORYWOUNDS = 0xc2,
	UNT_GD_SOULCOLD = 0xc3,
	UNT_GD_HAWKEYES = 0xc4,

	UNT_MAX = 0x190
};

/**
 * Skill Unit Save
 **/
void skill_usave_add(struct map_session_data * sd, uint16 skill_id, uint16 skill_lv);
void skill_usave_trigger(struct map_session_data *sd);

/**
 * Warlock
 **/
enum e_wl_spheres {
	WLS_FIRE = 0x44,
	WLS_WIND,
	WLS_WATER,
	WLS_STONE,
};

struct s_skill_spellbook_db {
	uint16 skill_id, points;
	t_itemid nameid;
};

class ReadingSpellbookDatabase : public TypesafeYamlDatabase<uint16, s_skill_spellbook_db> {
public:
	ReadingSpellbookDatabase() : TypesafeYamlDatabase("READING_SPELLBOOK_DB", 1) {

	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const YAML::Node& node);
	std::shared_ptr<s_skill_spellbook_db> findBook(t_itemid nameid);
};

extern ReadingSpellbookDatabase reading_spellbook_db;

void skill_spellbook(struct map_session_data *sd, t_itemid nameid);

int skill_block_check(struct block_list *bl, enum sc_type type, uint16 skill_id);

struct s_skill_magicmushroom_db {
	uint16 skill_id;
};

class MagicMushroomDatabase : public TypesafeYamlDatabase<uint16, s_skill_magicmushroom_db> {
public:
	MagicMushroomDatabase() : TypesafeYamlDatabase("MAGIC_MUSHROOM_DB", 1) {

	}

	const std::string getDefaultLocation();
	uint64 parseBodyNode(const YAML::Node &node);
};

extern MagicMushroomDatabase magic_mushroom_db;

int skill_maelstrom_suction(struct block_list *bl, va_list ap);
bool skill_check_shadowform(struct block_list *bl, int64 damage, int hit);

/**
 * Ranger
 **/
int skill_detonator(struct block_list *bl, va_list ap);
bool skill_check_camouflage(struct block_list *bl, struct status_change_entry *sce);

/**
 * Mechanic
 **/
int skill_magicdecoy(struct map_session_data *sd, t_itemid nameid, int skill_id);

int skill_summon_elemental_sphere(struct map_session_data *sd, t_itemid nameid, int skill_id);

int skill_plant_cultivation(struct map_session_data *sd, t_itemid nameid, int skill_id);
/**
 * Guiltoine Cross
 **/
int skill_poisoningweapon( struct map_session_data *sd, t_itemid nameid);

/**
 * Auto Shadow Spell (Shadow Chaser)
 **/
int skill_select_menu(struct map_session_data *sd,uint16 skill_id);

int skill_elementalanalysis(struct map_session_data *sd, int n, uint16 skill_lv, unsigned short *item_list); // Sorcerer Four Elemental Analisys.
int skill_changematerial(struct map_session_data *sd, int n, unsigned short *item_list);	// Genetic Change Material.
int skill_get_elemental_type(uint16 skill_id, uint16 skill_lv);

int skill_banding_count(struct map_session_data *sd);

int skill_is_combo(uint16 skill_id);
void skill_combo_toggle_inf(struct block_list* bl, uint16 skill_id, int inf);

void skill_reveal_trap_inarea(struct block_list *src, int range, int x, int y);
int skill_get_time3(struct map_data *mapdata, uint16 skill_id, uint16 skill_lv);
int skill_area_sub(struct block_list *bl, va_list ap);

/// Variable name of copied skill by Plagiarism
#define SKILL_VAR_PLAGIARISM "CLONE_SKILL"
/// Variable name of copied skill level by Plagiarism
#define SKILL_VAR_PLAGIARISM_LV "CLONE_SKILL_LV"

/// Variable name of copied skill by Reproduce
#define SKILL_VAR_REPRODUCE "REPRODUCE_SKILL"
/// Variable name of copied skill level by Reproduce
#define SKILL_VAR_REPRODUCE_LV "REPRODUCE_SKILL_LV"

#define SKILL_CHK_HOMUN(skill_id) ( (skill_id) >= HM_SKILLBASE && (skill_id) < HM_SKILLBASE+MAX_HOMUNSKILL )
#define SKILL_CHK_MERC(skill_id)  ( (skill_id) >= MC_SKILLBASE && (skill_id) < MC_SKILLBASE+MAX_MERCSKILL )
#define SKILL_CHK_ELEM(skill_id)  ( (skill_id) >= EL_SKILLBASE && (skill_id) < EL_SKILLBASE+MAX_ELEMENTALSKILL )
#define SKILL_CHK_GUILD(skill_id) ( (skill_id) >= GD_SKILLBASE && (skill_id) < GD_SKILLBASE+MAX_GUILDSKILL )

#endif /* SKILL_HPP */
