
SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


CREATE TABLE `acc_reg_num` (
  `account_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `key` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL DEFAULT '',
  `index` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `value` bigint(11) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `acc_reg_str` (
  `account_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `key` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL DEFAULT '',
  `index` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `value` varchar(254) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `achievement` (
  `char_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `id` bigint(11) UNSIGNED NOT NULL,
  `count1` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `count2` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `count3` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `count4` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `count5` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `count6` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `count7` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `count8` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `count9` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `count10` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `completed` datetime DEFAULT NULL,
  `rewarded` datetime DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `auction` (
  `auction_id` bigint(20) UNSIGNED NOT NULL,
  `seller_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `seller_name` varchar(30) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `buyer_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `buyer_name` varchar(30) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `price` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `buynow` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `hours` smallint(6) NOT NULL DEFAULT 0,
  `timestamp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `nameid` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `item_name` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `type` smallint(6) NOT NULL DEFAULT 0,
  `refine` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `attribute` tinyint(4) UNSIGNED NOT NULL DEFAULT 0,
  `card0` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card1` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card2` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card3` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `option_id0` smallint(5) NOT NULL DEFAULT 0,
  `option_val0` smallint(5) NOT NULL DEFAULT 0,
  `option_parm0` tinyint(3) NOT NULL DEFAULT 0,
  `option_id1` smallint(5) NOT NULL DEFAULT 0,
  `option_val1` smallint(5) NOT NULL DEFAULT 0,
  `option_parm1` tinyint(3) NOT NULL DEFAULT 0,
  `option_id2` smallint(5) NOT NULL DEFAULT 0,
  `option_val2` smallint(5) NOT NULL DEFAULT 0,
  `option_parm2` tinyint(3) NOT NULL DEFAULT 0,
  `option_id3` smallint(5) NOT NULL DEFAULT 0,
  `option_val3` smallint(5) NOT NULL DEFAULT 0,
  `option_parm3` tinyint(3) NOT NULL DEFAULT 0,
  `option_id4` smallint(5) NOT NULL DEFAULT 0,
  `option_val4` smallint(5) NOT NULL DEFAULT 0,
  `option_parm4` tinyint(3) NOT NULL DEFAULT 0,
  `unique_id` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `enchantgrade` tinyint(3) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `bonus_script` (
  `char_id` int(11) UNSIGNED NOT NULL,
  `script` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `tick` bigint(20) NOT NULL DEFAULT 0,
  `flag` smallint(5) UNSIGNED NOT NULL DEFAULT 0,
  `type` tinyint(1) UNSIGNED NOT NULL DEFAULT 0,
  `icon` smallint(3) NOT NULL DEFAULT -1
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `buyingstores` (
  `id` int(10) UNSIGNED NOT NULL,
  `account_id` int(11) UNSIGNED NOT NULL,
  `char_id` int(10) UNSIGNED NOT NULL,
  `sex` enum('F','M') COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'M',
  `map` varchar(20) COLLATE utf8mb4_unicode_ci NOT NULL,
  `x` smallint(5) UNSIGNED NOT NULL,
  `y` smallint(5) UNSIGNED NOT NULL,
  `title` varchar(80) COLLATE utf8mb4_unicode_ci NOT NULL,
  `limit` int(10) UNSIGNED NOT NULL,
  `body_direction` char(1) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '4',
  `head_direction` char(1) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '0',
  `sit` char(1) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '1',
  `autotrade` tinyint(4) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `buyingstore_items` (
  `buyingstore_id` int(10) UNSIGNED NOT NULL,
  `index` smallint(5) UNSIGNED NOT NULL,
  `item_id` int(10) UNSIGNED NOT NULL,
  `amount` smallint(5) UNSIGNED NOT NULL,
  `price` int(10) UNSIGNED NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `cart_inventory` (
  `id` int(11) NOT NULL,
  `char_id` int(11) NOT NULL DEFAULT 0,
  `nameid` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `amount` int(11) NOT NULL DEFAULT 0,
  `equip` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `identify` smallint(6) NOT NULL DEFAULT 0,
  `refine` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `attribute` tinyint(4) NOT NULL DEFAULT 0,
  `card0` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card1` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card2` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card3` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `option_id0` smallint(5) NOT NULL DEFAULT 0,
  `option_val0` smallint(5) NOT NULL DEFAULT 0,
  `option_parm0` tinyint(3) NOT NULL DEFAULT 0,
  `option_id1` smallint(5) NOT NULL DEFAULT 0,
  `option_val1` smallint(5) NOT NULL DEFAULT 0,
  `option_parm1` tinyint(3) NOT NULL DEFAULT 0,
  `option_id2` smallint(5) NOT NULL DEFAULT 0,
  `option_val2` smallint(5) NOT NULL DEFAULT 0,
  `option_parm2` tinyint(3) NOT NULL DEFAULT 0,
  `option_id3` smallint(5) NOT NULL DEFAULT 0,
  `option_val3` smallint(5) NOT NULL DEFAULT 0,
  `option_parm3` tinyint(3) NOT NULL DEFAULT 0,
  `option_id4` smallint(5) NOT NULL DEFAULT 0,
  `option_val4` smallint(5) NOT NULL DEFAULT 0,
  `option_parm4` tinyint(3) NOT NULL DEFAULT 0,
  `expire_time` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `bound` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `unique_id` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `enchantgrade` tinyint(3) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `char` (
  `char_id` int(11) UNSIGNED NOT NULL,
  `account_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `char_num` tinyint(1) NOT NULL DEFAULT 0,
  `name` varchar(30) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `class` smallint(6) UNSIGNED NOT NULL DEFAULT 0,
  `base_level` smallint(6) UNSIGNED NOT NULL DEFAULT 1,
  `job_level` smallint(6) UNSIGNED NOT NULL DEFAULT 1,
  `base_exp` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `job_exp` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `zeny` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `str` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `agi` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `vit` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `int` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `dex` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `luk` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `max_hp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `hp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `max_sp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `sp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `status_point` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `skill_point` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `option` int(11) NOT NULL DEFAULT 0,
  `karma` tinyint(3) NOT NULL DEFAULT 0,
  `manner` smallint(6) NOT NULL DEFAULT 0,
  `party_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `guild_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `pet_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `homun_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `elemental_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `hair` tinyint(4) UNSIGNED NOT NULL DEFAULT 0,
  `hair_color` smallint(5) UNSIGNED NOT NULL DEFAULT 0,
  `clothes_color` smallint(5) UNSIGNED NOT NULL DEFAULT 0,
  `body` smallint(5) UNSIGNED NOT NULL DEFAULT 0,
  `weapon` smallint(6) UNSIGNED NOT NULL DEFAULT 0,
  `shield` smallint(6) UNSIGNED NOT NULL DEFAULT 0,
  `head_top` smallint(6) UNSIGNED NOT NULL DEFAULT 0,
  `head_mid` smallint(6) UNSIGNED NOT NULL DEFAULT 0,
  `head_bottom` smallint(6) UNSIGNED NOT NULL DEFAULT 0,
  `robe` smallint(6) UNSIGNED NOT NULL DEFAULT 0,
  `last_map` varchar(11) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `last_x` smallint(4) UNSIGNED NOT NULL DEFAULT 53,
  `last_y` smallint(4) UNSIGNED NOT NULL DEFAULT 111,
  `save_map` varchar(11) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `save_x` smallint(4) UNSIGNED NOT NULL DEFAULT 53,
  `save_y` smallint(4) UNSIGNED NOT NULL DEFAULT 111,
  `partner_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `online` tinyint(2) NOT NULL DEFAULT 0,
  `father` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `mother` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `child` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `fame` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `rename` smallint(3) UNSIGNED NOT NULL DEFAULT 0,
  `delete_date` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `moves` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `unban_time` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `font` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `uniqueitem_counter` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `sex` enum('M','F') COLLATE utf8mb4_unicode_ci NOT NULL,
  `hotkey_rowshift` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `hotkey_rowshift2` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `clan_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `last_login` datetime DEFAULT NULL,
  `title_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `show_equip` tinyint(3) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `charlog` (
  `id` bigint(20) UNSIGNED NOT NULL,
  `time` datetime NOT NULL,
  `char_msg` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'char select',
  `account_id` int(11) NOT NULL DEFAULT 0,
  `char_num` tinyint(4) NOT NULL DEFAULT 0,
  `name` varchar(23) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `str` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `agi` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `vit` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `int` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `dex` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `luk` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `hair` tinyint(4) NOT NULL DEFAULT 0,
  `hair_color` int(11) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `char_reg_num` (
  `char_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `key` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL DEFAULT '',
  `index` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `value` bigint(11) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `char_reg_str` (
  `char_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `key` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL DEFAULT '',
  `index` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `value` varchar(254) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `clan` (
  `clan_id` int(11) UNSIGNED NOT NULL,
  `name` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `master` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `mapname` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `max_member` smallint(6) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `clan_alliance` (
  `clan_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `opposition` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `alliance_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `name` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `db_roulette` (
  `index` int(11) NOT NULL DEFAULT 0,
  `level` smallint(5) UNSIGNED NOT NULL,
  `item_id` int(10) UNSIGNED NOT NULL,
  `amount` smallint(5) UNSIGNED NOT NULL DEFAULT 1,
  `flag` smallint(5) UNSIGNED NOT NULL DEFAULT 1
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `elemental` (
  `ele_id` int(11) UNSIGNED NOT NULL,
  `char_id` int(11) NOT NULL,
  `class` mediumint(9) UNSIGNED NOT NULL DEFAULT 0,
  `mode` int(11) UNSIGNED NOT NULL DEFAULT 1,
  `hp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `sp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `max_hp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `max_sp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `atk1` mediumint(6) UNSIGNED NOT NULL DEFAULT 0,
  `atk2` mediumint(6) UNSIGNED NOT NULL DEFAULT 0,
  `matk` mediumint(6) UNSIGNED NOT NULL DEFAULT 0,
  `aspd` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `def` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `mdef` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `flee` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `hit` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `life_time` bigint(20) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `friends` (
  `char_id` int(11) NOT NULL DEFAULT 0,
  `friend_id` int(11) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `global_acc_reg_num` (
  `account_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `key` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL DEFAULT '',
  `index` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `value` bigint(11) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `global_acc_reg_str` (
  `account_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `key` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL DEFAULT '',
  `index` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `value` varchar(254) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `guild` (
  `guild_id` int(11) UNSIGNED NOT NULL,
  `name` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `char_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `master` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `guild_lv` tinyint(6) UNSIGNED NOT NULL DEFAULT 0,
  `connect_member` tinyint(6) UNSIGNED NOT NULL DEFAULT 0,
  `max_member` tinyint(6) UNSIGNED NOT NULL DEFAULT 0,
  `average_lv` smallint(6) UNSIGNED NOT NULL DEFAULT 1,
  `exp` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `next_exp` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `skill_point` tinyint(11) UNSIGNED NOT NULL DEFAULT 0,
  `mes1` varchar(60) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `mes2` varchar(120) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `emblem_len` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `emblem_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `emblem_data` blob DEFAULT NULL,
  `last_master_change` datetime DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `guild_alliance` (
  `guild_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `opposition` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `alliance_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `name` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `guild_castle` (
  `castle_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `guild_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `economy` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `defense` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `triggerE` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `triggerD` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `nextTime` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `payTime` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `createTime` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `visibleC` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `visibleG0` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `visibleG1` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `visibleG2` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `visibleG3` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `visibleG4` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `visibleG5` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `visibleG6` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `visibleG7` int(11) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `guild_expulsion` (
  `guild_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `account_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `name` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `mes` varchar(40) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `guild_member` (
  `guild_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `char_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `exp` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `position` tinyint(6) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `guild_position` (
  `guild_id` int(9) UNSIGNED NOT NULL DEFAULT 0,
  `position` tinyint(6) UNSIGNED NOT NULL DEFAULT 0,
  `name` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `mode` smallint(11) UNSIGNED NOT NULL DEFAULT 0,
  `exp_mode` tinyint(11) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `guild_skill` (
  `guild_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `id` smallint(11) UNSIGNED NOT NULL DEFAULT 0,
  `lv` tinyint(11) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `guild_storage` (
  `id` int(10) UNSIGNED NOT NULL,
  `guild_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `nameid` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `amount` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `equip` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `identify` smallint(6) UNSIGNED NOT NULL DEFAULT 0,
  `refine` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `attribute` tinyint(4) UNSIGNED NOT NULL DEFAULT 0,
  `card0` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card1` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card2` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card3` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `option_id0` smallint(5) NOT NULL DEFAULT 0,
  `option_val0` smallint(5) NOT NULL DEFAULT 0,
  `option_parm0` tinyint(3) NOT NULL DEFAULT 0,
  `option_id1` smallint(5) NOT NULL DEFAULT 0,
  `option_val1` smallint(5) NOT NULL DEFAULT 0,
  `option_parm1` tinyint(3) NOT NULL DEFAULT 0,
  `option_id2` smallint(5) NOT NULL DEFAULT 0,
  `option_val2` smallint(5) NOT NULL DEFAULT 0,
  `option_parm2` tinyint(3) NOT NULL DEFAULT 0,
  `option_id3` smallint(5) NOT NULL DEFAULT 0,
  `option_val3` smallint(5) NOT NULL DEFAULT 0,
  `option_parm3` tinyint(3) NOT NULL DEFAULT 0,
  `option_id4` smallint(5) NOT NULL DEFAULT 0,
  `option_val4` smallint(5) NOT NULL DEFAULT 0,
  `option_parm4` tinyint(3) NOT NULL DEFAULT 0,
  `expire_time` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `bound` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `unique_id` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `enchantgrade` tinyint(3) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `guild_storage_log` (
  `id` int(11) NOT NULL,
  `guild_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `time` datetime NOT NULL,
  `char_id` int(11) NOT NULL DEFAULT 0,
  `name` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `nameid` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `amount` int(11) NOT NULL DEFAULT 1,
  `identify` smallint(6) NOT NULL DEFAULT 0,
  `refine` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `attribute` tinyint(4) UNSIGNED NOT NULL DEFAULT 0,
  `card0` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card1` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card2` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card3` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `option_id0` smallint(5) NOT NULL DEFAULT 0,
  `option_val0` smallint(5) NOT NULL DEFAULT 0,
  `option_parm0` tinyint(3) NOT NULL DEFAULT 0,
  `option_id1` smallint(5) NOT NULL DEFAULT 0,
  `option_val1` smallint(5) NOT NULL DEFAULT 0,
  `option_parm1` tinyint(3) NOT NULL DEFAULT 0,
  `option_id2` smallint(5) NOT NULL DEFAULT 0,
  `option_val2` smallint(5) NOT NULL DEFAULT 0,
  `option_parm2` tinyint(3) NOT NULL DEFAULT 0,
  `option_id3` smallint(5) NOT NULL DEFAULT 0,
  `option_val3` smallint(5) NOT NULL DEFAULT 0,
  `option_parm3` tinyint(3) NOT NULL DEFAULT 0,
  `option_id4` smallint(5) NOT NULL DEFAULT 0,
  `option_val4` smallint(5) NOT NULL DEFAULT 0,
  `option_parm4` tinyint(3) NOT NULL DEFAULT 0,
  `expire_time` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `unique_id` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `bound` tinyint(1) UNSIGNED NOT NULL DEFAULT 0,
  `enchantgrade` tinyint(3) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `homunculus` (
  `homun_id` int(11) NOT NULL,
  `char_id` int(11) NOT NULL,
  `class` mediumint(9) UNSIGNED NOT NULL DEFAULT 0,
  `prev_class` mediumint(9) NOT NULL DEFAULT 0,
  `name` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `level` smallint(4) NOT NULL DEFAULT 0,
  `exp` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `intimacy` int(12) NOT NULL DEFAULT 0,
  `hunger` smallint(4) NOT NULL DEFAULT 0,
  `str` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `agi` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `vit` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `int` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `dex` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `luk` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `hp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `max_hp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `sp` int(11) NOT NULL DEFAULT 0,
  `max_sp` int(11) NOT NULL DEFAULT 0,
  `skill_point` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `alive` tinyint(2) NOT NULL DEFAULT 1,
  `rename_flag` tinyint(2) NOT NULL DEFAULT 0,
  `vaporize` tinyint(2) NOT NULL DEFAULT 0,
  `autofeed` tinyint(2) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `hotkey` (
  `char_id` int(11) NOT NULL,
  `hotkey` tinyint(2) UNSIGNED NOT NULL,
  `type` tinyint(1) UNSIGNED NOT NULL DEFAULT 0,
  `itemskill_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `skill_lvl` tinyint(4) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `interlog` (
  `id` int(11) NOT NULL,
  `time` datetime NOT NULL,
  `log` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `inventory` (
  `id` int(11) UNSIGNED NOT NULL,
  `char_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `nameid` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `amount` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `equip` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `identify` smallint(6) NOT NULL DEFAULT 0,
  `refine` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `attribute` tinyint(4) UNSIGNED NOT NULL DEFAULT 0,
  `card0` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card1` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card2` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card3` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `option_id0` smallint(5) NOT NULL DEFAULT 0,
  `option_val0` smallint(5) NOT NULL DEFAULT 0,
  `option_parm0` tinyint(3) NOT NULL DEFAULT 0,
  `option_id1` smallint(5) NOT NULL DEFAULT 0,
  `option_val1` smallint(5) NOT NULL DEFAULT 0,
  `option_parm1` tinyint(3) NOT NULL DEFAULT 0,
  `option_id2` smallint(5) NOT NULL DEFAULT 0,
  `option_val2` smallint(5) NOT NULL DEFAULT 0,
  `option_parm2` tinyint(3) NOT NULL DEFAULT 0,
  `option_id3` smallint(5) NOT NULL DEFAULT 0,
  `option_val3` smallint(5) NOT NULL DEFAULT 0,
  `option_parm3` tinyint(3) NOT NULL DEFAULT 0,
  `option_id4` smallint(5) NOT NULL DEFAULT 0,
  `option_val4` smallint(5) NOT NULL DEFAULT 0,
  `option_parm4` tinyint(3) NOT NULL DEFAULT 0,
  `expire_time` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `favorite` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `bound` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `unique_id` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `equip_switch` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `enchantgrade` tinyint(3) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `ipbanlist` (
  `list` varchar(15) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `btime` datetime NOT NULL,
  `rtime` datetime NOT NULL,
  `reason` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `item_db2_re` (
  `id` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `name_aegis` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `name_english` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `type` varchar(20) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `subtype` varchar(20) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `price_buy` mediumint(8) UNSIGNED DEFAULT NULL,
  `price_sell` mediumint(8) UNSIGNED DEFAULT NULL,
  `weight` smallint(5) UNSIGNED DEFAULT NULL,
  `attack` smallint(5) UNSIGNED DEFAULT NULL,
  `magic_attack` smallint(5) UNSIGNED DEFAULT NULL,
  `defense` smallint(5) UNSIGNED DEFAULT NULL,
  `range` tinyint(2) UNSIGNED DEFAULT NULL,
  `slots` tinyint(2) UNSIGNED DEFAULT NULL,
  `job_all` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_acolyte` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_alchemist` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_archer` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_assassin` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_barddancer` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_blacksmith` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_crusader` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_gunslinger` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_hunter` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_kagerouoboro` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_knight` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_mage` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_merchant` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_monk` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_ninja` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_novice` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_priest` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_rebellion` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_rogue` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_sage` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_soullinker` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_stargladiator` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_summoner` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_supernovice` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_swordman` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_taekwon` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_thief` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_wizard` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_all` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_normal` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_upper` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_baby` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_third` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_third_upper` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_third_baby` tinyint(1) UNSIGNED DEFAULT NULL,
  `gender` varchar(10) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `location_head_top` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_head_mid` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_head_low` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_armor` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_right_hand` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_left_hand` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_garment` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shoes` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_right_accessory` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_left_accessory` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_costume_head_top` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_costume_head_mid` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_costume_head_low` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_costume_garment` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_ammo` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shadow_armor` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shadow_weapon` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shadow_shield` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shadow_shoes` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shadow_right_accessory` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shadow_left_accessory` tinyint(1) UNSIGNED DEFAULT NULL,
  `weapon_level` tinyint(1) UNSIGNED DEFAULT NULL,
  `equip_level_min` tinyint(3) UNSIGNED DEFAULT NULL,
  `equip_level_max` tinyint(3) UNSIGNED DEFAULT NULL,
  `refineable` tinyint(1) UNSIGNED DEFAULT NULL,
  `view` smallint(5) UNSIGNED DEFAULT NULL,
  `alias_name` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `flag_buyingstore` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_deadbranch` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_container` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_uniqueid` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_bindonequip` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_dropannounce` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_noconsume` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_dropeffect` varchar(20) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `delay_duration` bigint(20) UNSIGNED DEFAULT NULL,
  `delay_status` varchar(30) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `stack_amount` smallint(5) UNSIGNED DEFAULT NULL,
  `stack_inventory` tinyint(1) UNSIGNED DEFAULT NULL,
  `stack_cart` tinyint(1) UNSIGNED DEFAULT NULL,
  `stack_storage` tinyint(1) UNSIGNED DEFAULT NULL,
  `stack_guildstorage` tinyint(1) UNSIGNED DEFAULT NULL,
  `nouse_override` smallint(5) UNSIGNED DEFAULT NULL,
  `nouse_sitting` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_override` smallint(5) UNSIGNED DEFAULT NULL,
  `trade_nodrop` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_notrade` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_tradepartner` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_nosell` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_nocart` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_nostorage` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_noguildstorage` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_nomail` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_noauction` tinyint(1) UNSIGNED DEFAULT NULL,
  `script` text COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `equip_script` text COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `unequip_script` text COLLATE utf8mb4_unicode_ci DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `item_db_re` (
  `id` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `name_aegis` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `name_english` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `type` varchar(20) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `subtype` varchar(20) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `price_buy` mediumint(8) UNSIGNED DEFAULT NULL,
  `price_sell` mediumint(8) UNSIGNED DEFAULT NULL,
  `weight` smallint(5) UNSIGNED DEFAULT NULL,
  `attack` smallint(5) UNSIGNED DEFAULT NULL,
  `magic_attack` smallint(5) UNSIGNED DEFAULT NULL,
  `defense` smallint(5) UNSIGNED DEFAULT NULL,
  `range` tinyint(2) UNSIGNED DEFAULT NULL,
  `slots` tinyint(2) UNSIGNED DEFAULT NULL,
  `job_all` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_acolyte` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_alchemist` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_archer` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_assassin` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_barddancer` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_blacksmith` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_crusader` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_gunslinger` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_hunter` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_kagerouoboro` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_knight` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_mage` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_merchant` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_monk` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_ninja` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_novice` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_priest` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_rebellion` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_rogue` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_sage` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_soullinker` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_stargladiator` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_summoner` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_supernovice` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_swordman` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_taekwon` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_thief` tinyint(1) UNSIGNED DEFAULT NULL,
  `job_wizard` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_all` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_normal` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_upper` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_baby` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_third` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_third_upper` tinyint(1) UNSIGNED DEFAULT NULL,
  `class_third_baby` tinyint(1) UNSIGNED DEFAULT NULL,
  `gender` varchar(10) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `location_head_top` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_head_mid` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_head_low` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_armor` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_right_hand` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_left_hand` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_garment` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shoes` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_right_accessory` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_left_accessory` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_costume_head_top` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_costume_head_mid` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_costume_head_low` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_costume_garment` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_ammo` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shadow_armor` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shadow_weapon` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shadow_shield` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shadow_shoes` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shadow_right_accessory` tinyint(1) UNSIGNED DEFAULT NULL,
  `location_shadow_left_accessory` tinyint(1) UNSIGNED DEFAULT NULL,
  `weapon_level` tinyint(1) UNSIGNED DEFAULT NULL,
  `equip_level_min` tinyint(3) UNSIGNED DEFAULT NULL,
  `equip_level_max` tinyint(3) UNSIGNED DEFAULT NULL,
  `refineable` tinyint(1) UNSIGNED DEFAULT NULL,
  `view` smallint(5) UNSIGNED DEFAULT NULL,
  `alias_name` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `flag_buyingstore` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_deadbranch` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_container` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_uniqueid` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_bindonequip` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_dropannounce` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_noconsume` tinyint(1) UNSIGNED DEFAULT NULL,
  `flag_dropeffect` varchar(20) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `delay_duration` bigint(20) UNSIGNED DEFAULT NULL,
  `delay_status` varchar(30) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `stack_amount` smallint(5) UNSIGNED DEFAULT NULL,
  `stack_inventory` tinyint(1) UNSIGNED DEFAULT NULL,
  `stack_cart` tinyint(1) UNSIGNED DEFAULT NULL,
  `stack_storage` tinyint(1) UNSIGNED DEFAULT NULL,
  `stack_guildstorage` tinyint(1) UNSIGNED DEFAULT NULL,
  `nouse_override` smallint(5) UNSIGNED DEFAULT NULL,
  `nouse_sitting` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_override` smallint(5) UNSIGNED DEFAULT NULL,
  `trade_nodrop` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_notrade` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_tradepartner` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_nosell` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_nocart` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_nostorage` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_noguildstorage` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_nomail` tinyint(1) UNSIGNED DEFAULT NULL,
  `trade_noauction` tinyint(1) UNSIGNED DEFAULT NULL,
  `script` text COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `equip_script` text COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `unequip_script` text COLLATE utf8mb4_unicode_ci DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `login` (
  `account_id` int(11) UNSIGNED NOT NULL,
  `userid` varchar(23) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `user_pass` varchar(32) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `sex` enum('M','F','S') COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'M',
  `email` varchar(39) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `group_id` tinyint(3) NOT NULL DEFAULT 0,
  `state` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `unban_time` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `expiration_time` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `logincount` mediumint(9) UNSIGNED NOT NULL DEFAULT 0,
  `lastlogin` datetime DEFAULT NULL,
  `last_ip` varchar(100) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `birthdate` date DEFAULT NULL,
  `character_slots` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `pincode` varchar(4) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `pincode_change` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `vip_time` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `old_group` tinyint(3) NOT NULL DEFAULT 0,
  `web_auth_token` varchar(17) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `web_auth_token_enabled` tinyint(2) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`, `group_id`, `state`, `unban_time`, `expiration_time`, `logincount`, `lastlogin`, `last_ip`, `birthdate`, `character_slots`, `pincode`, `pincode_change`, `vip_time`, `old_group`, `web_auth_token`, `web_auth_token_enabled`) VALUES
(1, 'inter_user', 'fba1bf81cf214153dcdd484b02520be4', 'S', 'athena@athena.com', 0, 0, 0, 0, 4166, '2023-04-17 13:51:50', '127.0.0.1', NULL, 0, '', 0, 0, 0, NULL, 0),
(2000000, 'admin', 'e10adc3949ba59abbe56e057f20f883e', 'M', 'a@a.com', 99, 0, 0, 0, 198, '2023-04-16 12:11:39', '127.0.0.1', NULL, 15, '', 0, 0, 0, NULL, 0),
(2000001, 'joangr', 'e10adc3949ba59abbe56e057f20f883e', 'M', 'a@a.com', 1, 0, 0, 0, 7119, '2023-04-17 14:41:56', '127.0.0.1', NULL, 15, '', 0, 0, 0, '4ac975eb6587fbed', 0);



CREATE TABLE `mail` (
  `id` bigint(20) UNSIGNED NOT NULL,
  `send_name` varchar(30) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `send_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `dest_name` varchar(30) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `dest_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `title` varchar(45) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `message` varchar(500) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `time` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `status` tinyint(2) NOT NULL DEFAULT 0,
  `zeny` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `type` smallint(5) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `mail_attachments` (
  `id` bigint(20) UNSIGNED NOT NULL,
  `index` smallint(5) UNSIGNED NOT NULL DEFAULT 0,
  `nameid` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `amount` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `refine` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `attribute` tinyint(4) UNSIGNED NOT NULL DEFAULT 0,
  `identify` smallint(6) NOT NULL DEFAULT 0,
  `card0` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card1` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card2` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card3` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `option_id0` smallint(5) NOT NULL DEFAULT 0,
  `option_val0` smallint(5) NOT NULL DEFAULT 0,
  `option_parm0` tinyint(3) NOT NULL DEFAULT 0,
  `option_id1` smallint(5) NOT NULL DEFAULT 0,
  `option_val1` smallint(5) NOT NULL DEFAULT 0,
  `option_parm1` tinyint(3) NOT NULL DEFAULT 0,
  `option_id2` smallint(5) NOT NULL DEFAULT 0,
  `option_val2` smallint(5) NOT NULL DEFAULT 0,
  `option_parm2` tinyint(3) NOT NULL DEFAULT 0,
  `option_id3` smallint(5) NOT NULL DEFAULT 0,
  `option_val3` smallint(5) NOT NULL DEFAULT 0,
  `option_parm3` tinyint(3) NOT NULL DEFAULT 0,
  `option_id4` smallint(5) NOT NULL DEFAULT 0,
  `option_val4` smallint(5) NOT NULL DEFAULT 0,
  `option_parm4` tinyint(3) NOT NULL DEFAULT 0,
  `unique_id` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `bound` tinyint(1) UNSIGNED NOT NULL DEFAULT 0,
  `enchantgrade` tinyint(3) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `mapreg` (
  `varname` varchar(32) CHARACTER SET utf8mb4 COLLATE utf8mb4_bin NOT NULL,
  `index` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `value` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `market` (
  `name` varchar(50) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `nameid` int(10) UNSIGNED NOT NULL,
  `price` int(11) UNSIGNED NOT NULL,
  `amount` smallint(5) UNSIGNED NOT NULL,
  `flag` tinyint(2) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `memo` (
  `memo_id` int(11) UNSIGNED NOT NULL,
  `char_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `map` varchar(11) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `x` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `y` smallint(4) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `mercenary` (
  `mer_id` int(11) UNSIGNED NOT NULL,
  `char_id` int(11) NOT NULL,
  `class` mediumint(9) UNSIGNED NOT NULL DEFAULT 0,
  `hp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `sp` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `kill_counter` int(11) NOT NULL,
  `life_time` bigint(20) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `mercenary_owner` (
  `char_id` int(11) NOT NULL,
  `merc_id` int(11) NOT NULL DEFAULT 0,
  `arch_calls` int(11) NOT NULL DEFAULT 0,
  `arch_faith` int(11) NOT NULL DEFAULT 0,
  `spear_calls` int(11) NOT NULL DEFAULT 0,
  `spear_faith` int(11) NOT NULL DEFAULT 0,
  `sword_calls` int(11) NOT NULL DEFAULT 0,
  `sword_faith` int(11) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `mob_db2_re` (
  `id` int(11) UNSIGNED NOT NULL,
  `name_aegis` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL,
  `name_english` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `name_japanese` text COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `level` smallint(6) UNSIGNED DEFAULT NULL,
  `hp` int(11) UNSIGNED DEFAULT NULL,
  `sp` mediumint(9) UNSIGNED DEFAULT NULL,
  `base_exp` int(11) UNSIGNED DEFAULT NULL,
  `job_exp` int(11) UNSIGNED DEFAULT NULL,
  `mvp_exp` int(11) UNSIGNED DEFAULT NULL,
  `attack` smallint(6) UNSIGNED DEFAULT NULL,
  `attack2` smallint(6) UNSIGNED DEFAULT NULL,
  `defense` smallint(6) UNSIGNED DEFAULT NULL,
  `magic_defense` smallint(6) UNSIGNED DEFAULT NULL,
  `str` smallint(6) UNSIGNED DEFAULT NULL,
  `agi` smallint(6) UNSIGNED DEFAULT NULL,
  `vit` smallint(6) UNSIGNED DEFAULT NULL,
  `int` smallint(6) UNSIGNED DEFAULT NULL,
  `dex` smallint(6) UNSIGNED DEFAULT NULL,
  `luk` smallint(6) UNSIGNED DEFAULT NULL,
  `attack_range` tinyint(4) UNSIGNED DEFAULT NULL,
  `skill_range` tinyint(4) UNSIGNED DEFAULT NULL,
  `chase_range` tinyint(4) UNSIGNED DEFAULT NULL,
  `size` varchar(24) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `race` varchar(24) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `racegroup_goblin` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_kobold` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_orc` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_golem` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_guardian` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_ninja` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_gvg` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_battlefield` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_treasure` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_biolab` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_manuk` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_splendide` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_scaraba` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_ogh_atk_def` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_ogh_hidden` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_bio5_swordman_thief` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_bio5_acolyte_merchant` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_bio5_mage_archer` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_bio5_mvp` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_clocktower` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_thanatos` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_faceworm` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_hearthunter` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_rockridge` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_werner_lab` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_temple_demon` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_illusion_vampire` tinyint(1) UNSIGNED DEFAULT NULL,
  `element` varchar(24) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `element_level` tinyint(4) UNSIGNED DEFAULT NULL,
  `walk_speed` smallint(6) UNSIGNED DEFAULT NULL,
  `attack_delay` smallint(6) UNSIGNED DEFAULT NULL,
  `attack_motion` smallint(6) UNSIGNED DEFAULT NULL,
  `damage_motion` smallint(6) UNSIGNED DEFAULT NULL,
  `damage_taken` smallint(6) UNSIGNED DEFAULT NULL,
  `ai` varchar(2) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `class` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mode_canmove` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_looter` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_aggressive` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_assist` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_castsensoridle` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_norandomwalk` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_nocast` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_canattack` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_castsensorchase` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_changechase` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_angry` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_changetargetmelee` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_changetargetchase` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_targetweak` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_randomtarget` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_ignoremelee` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_ignoremagic` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_ignoreranged` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_mvp` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_ignoremisc` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_knockbackimmune` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_teleportblock` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_fixeditemdrop` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_detector` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_statusimmune` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_skillimmune` tinyint(1) UNSIGNED DEFAULT NULL,
  `mvpdrop1_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mvpdrop1_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `mvpdrop1_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mvpdrop1_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `mvpdrop2_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mvpdrop2_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `mvpdrop2_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mvpdrop2_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `mvpdrop3_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mvpdrop3_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `mvpdrop3_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mvpdrop3_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop1_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop1_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop1_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop1_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop1_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop2_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop2_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop2_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop2_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop2_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop3_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop3_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop3_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop3_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop3_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop4_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop4_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop4_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop4_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop4_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop5_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop5_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop5_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop5_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop5_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop6_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop6_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop6_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop6_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop6_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop7_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop7_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop7_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop7_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop7_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop8_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop8_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop8_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop8_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop8_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop9_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop9_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop9_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop9_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop9_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop10_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop10_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop10_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop10_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop10_index` tinyint(2) UNSIGNED DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `mob_db_re` (
  `id` int(11) UNSIGNED NOT NULL,
  `name_aegis` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL,
  `name_english` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `name_japanese` text COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `level` smallint(6) UNSIGNED DEFAULT NULL,
  `hp` int(11) UNSIGNED DEFAULT NULL,
  `sp` mediumint(9) UNSIGNED DEFAULT NULL,
  `base_exp` int(11) UNSIGNED DEFAULT NULL,
  `job_exp` int(11) UNSIGNED DEFAULT NULL,
  `mvp_exp` int(11) UNSIGNED DEFAULT NULL,
  `attack` smallint(6) UNSIGNED DEFAULT NULL,
  `attack2` smallint(6) UNSIGNED DEFAULT NULL,
  `defense` smallint(6) UNSIGNED DEFAULT NULL,
  `magic_defense` smallint(6) UNSIGNED DEFAULT NULL,
  `str` smallint(6) UNSIGNED DEFAULT NULL,
  `agi` smallint(6) UNSIGNED DEFAULT NULL,
  `vit` smallint(6) UNSIGNED DEFAULT NULL,
  `int` smallint(6) UNSIGNED DEFAULT NULL,
  `dex` smallint(6) UNSIGNED DEFAULT NULL,
  `luk` smallint(6) UNSIGNED DEFAULT NULL,
  `attack_range` tinyint(4) UNSIGNED DEFAULT NULL,
  `skill_range` tinyint(4) UNSIGNED DEFAULT NULL,
  `chase_range` tinyint(4) UNSIGNED DEFAULT NULL,
  `size` varchar(24) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `race` varchar(24) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `racegroup_goblin` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_kobold` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_orc` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_golem` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_guardian` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_ninja` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_gvg` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_battlefield` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_treasure` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_biolab` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_manuk` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_splendide` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_scaraba` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_ogh_atk_def` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_ogh_hidden` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_bio5_swordman_thief` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_bio5_acolyte_merchant` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_bio5_mage_archer` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_bio5_mvp` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_clocktower` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_thanatos` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_faceworm` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_hearthunter` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_rockridge` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_werner_lab` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_temple_demon` tinyint(1) UNSIGNED DEFAULT NULL,
  `racegroup_illusion_vampire` tinyint(1) UNSIGNED DEFAULT NULL,
  `element` varchar(24) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `element_level` tinyint(4) UNSIGNED DEFAULT NULL,
  `walk_speed` smallint(6) UNSIGNED DEFAULT NULL,
  `attack_delay` smallint(6) UNSIGNED DEFAULT NULL,
  `attack_motion` smallint(6) UNSIGNED DEFAULT NULL,
  `damage_motion` smallint(6) UNSIGNED DEFAULT NULL,
  `damage_taken` smallint(6) UNSIGNED DEFAULT NULL,
  `ai` varchar(2) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `class` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mode_canmove` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_looter` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_aggressive` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_assist` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_castsensoridle` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_norandomwalk` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_nocast` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_canattack` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_castsensorchase` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_changechase` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_angry` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_changetargetmelee` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_changetargetchase` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_targetweak` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_randomtarget` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_ignoremelee` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_ignoremagic` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_ignoreranged` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_mvp` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_ignoremisc` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_knockbackimmune` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_teleportblock` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_fixeditemdrop` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_detector` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_statusimmune` tinyint(1) UNSIGNED DEFAULT NULL,
  `mode_skillimmune` tinyint(1) UNSIGNED DEFAULT NULL,
  `mvpdrop1_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mvpdrop1_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `mvpdrop1_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mvpdrop1_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `mvpdrop2_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mvpdrop2_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `mvpdrop2_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mvpdrop2_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `mvpdrop3_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mvpdrop3_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `mvpdrop3_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `mvpdrop3_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop1_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop1_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop1_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop1_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop1_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop2_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop2_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop2_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop2_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop2_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop3_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop3_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop3_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop3_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop3_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop4_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop4_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop4_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop4_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop4_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop5_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop5_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop5_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop5_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop5_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop6_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop6_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop6_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop6_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop6_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop7_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop7_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop7_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop7_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop7_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop8_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop8_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop8_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop8_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop8_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop9_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop9_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop9_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop9_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop9_index` tinyint(2) UNSIGNED DEFAULT NULL,
  `drop10_item` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop10_rate` smallint(9) UNSIGNED DEFAULT NULL,
  `drop10_nosteal` tinyint(1) UNSIGNED DEFAULT NULL,
  `drop10_option` varchar(50) COLLATE utf8mb4_unicode_ci DEFAULT NULL,
  `drop10_index` tinyint(2) UNSIGNED DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `party` (
  `party_id` int(11) UNSIGNED NOT NULL,
  `name` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `exp` tinyint(11) UNSIGNED NOT NULL DEFAULT 0,
  `item` tinyint(11) UNSIGNED NOT NULL DEFAULT 0,
  `leader_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `leader_char` int(11) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `pet` (
  `pet_id` int(11) UNSIGNED NOT NULL,
  `class` mediumint(9) UNSIGNED NOT NULL DEFAULT 0,
  `name` varchar(24) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `account_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `char_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `level` smallint(4) UNSIGNED NOT NULL DEFAULT 0,
  `egg_id` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `equip` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `intimate` smallint(9) UNSIGNED NOT NULL DEFAULT 0,
  `hungry` smallint(9) UNSIGNED NOT NULL DEFAULT 0,
  `rename_flag` tinyint(4) UNSIGNED NOT NULL DEFAULT 0,
  `incubate` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `autofeed` tinyint(2) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `quest` (
  `char_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `quest_id` int(10) UNSIGNED NOT NULL,
  `state` enum('0','1','2') COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '0',
  `time` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `count1` mediumint(8) UNSIGNED NOT NULL DEFAULT 0,
  `count2` mediumint(8) UNSIGNED NOT NULL DEFAULT 0,
  `count3` mediumint(8) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `sales` (
  `nameid` int(10) UNSIGNED NOT NULL,
  `start` datetime NOT NULL,
  `end` datetime NOT NULL,
  `amount` int(11) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `sc_data` (
  `account_id` int(11) UNSIGNED NOT NULL,
  `char_id` int(11) UNSIGNED NOT NULL,
  `type` smallint(11) UNSIGNED NOT NULL,
  `tick` bigint(20) NOT NULL,
  `val1` int(11) NOT NULL DEFAULT 0,
  `val2` int(11) NOT NULL DEFAULT 0,
  `val3` int(11) NOT NULL DEFAULT 0,
  `val4` int(11) NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `skill` (
  `char_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `id` smallint(11) UNSIGNED NOT NULL DEFAULT 0,
  `lv` tinyint(4) UNSIGNED NOT NULL DEFAULT 0,
  `flag` tinyint(1) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `skillcooldown` (
  `account_id` int(11) UNSIGNED NOT NULL,
  `char_id` int(11) UNSIGNED NOT NULL,
  `skill` smallint(11) UNSIGNED NOT NULL DEFAULT 0,
  `tick` bigint(20) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `skill_homunculus` (
  `homun_id` int(11) NOT NULL,
  `id` int(11) NOT NULL,
  `lv` smallint(6) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `storage` (
  `id` int(11) UNSIGNED NOT NULL,
  `account_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `nameid` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `amount` smallint(11) UNSIGNED NOT NULL DEFAULT 0,
  `equip` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `identify` smallint(6) UNSIGNED NOT NULL DEFAULT 0,
  `refine` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `attribute` tinyint(4) UNSIGNED NOT NULL DEFAULT 0,
  `card0` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card1` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card2` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `card3` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `option_id0` smallint(5) NOT NULL DEFAULT 0,
  `option_val0` smallint(5) NOT NULL DEFAULT 0,
  `option_parm0` tinyint(3) NOT NULL DEFAULT 0,
  `option_id1` smallint(5) NOT NULL DEFAULT 0,
  `option_val1` smallint(5) NOT NULL DEFAULT 0,
  `option_parm1` tinyint(3) NOT NULL DEFAULT 0,
  `option_id2` smallint(5) NOT NULL DEFAULT 0,
  `option_val2` smallint(5) NOT NULL DEFAULT 0,
  `option_parm2` tinyint(3) NOT NULL DEFAULT 0,
  `option_id3` smallint(5) NOT NULL DEFAULT 0,
  `option_val3` smallint(5) NOT NULL DEFAULT 0,
  `option_parm3` tinyint(3) NOT NULL DEFAULT 0,
  `option_id4` smallint(5) NOT NULL DEFAULT 0,
  `option_val4` smallint(5) NOT NULL DEFAULT 0,
  `option_parm4` tinyint(3) NOT NULL DEFAULT 0,
  `expire_time` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `bound` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
  `unique_id` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `enchantgrade` tinyint(3) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `vendings` (
  `id` int(10) UNSIGNED NOT NULL,
  `account_id` int(11) UNSIGNED NOT NULL,
  `char_id` int(10) UNSIGNED NOT NULL,
  `sex` enum('F','M') COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'M',
  `map` varchar(20) COLLATE utf8mb4_unicode_ci NOT NULL,
  `x` smallint(5) UNSIGNED NOT NULL,
  `y` smallint(5) UNSIGNED NOT NULL,
  `title` varchar(80) COLLATE utf8mb4_unicode_ci NOT NULL,
  `body_direction` char(1) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '4',
  `head_direction` char(1) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '0',
  `sit` char(1) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '1',
  `autotrade` tinyint(4) NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `vending_items` (
  `vending_id` int(10) UNSIGNED NOT NULL,
  `index` smallint(5) UNSIGNED NOT NULL,
  `cartinventory_id` int(10) UNSIGNED NOT NULL,
  `amount` smallint(5) UNSIGNED NOT NULL,
  `price` int(10) UNSIGNED NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


ALTER TABLE `acc_reg_num`
  ADD PRIMARY KEY (`account_id`,`key`,`index`),
  ADD KEY `account_id` (`account_id`);


ALTER TABLE `acc_reg_str`
  ADD PRIMARY KEY (`account_id`,`key`,`index`),
  ADD KEY `account_id` (`account_id`);


ALTER TABLE `achievement`
  ADD PRIMARY KEY (`char_id`,`id`),
  ADD KEY `char_id` (`char_id`);


ALTER TABLE `auction`
  ADD PRIMARY KEY (`auction_id`);

ALTER TABLE `bonus_script`
  ADD PRIMARY KEY (`char_id`,`type`);


ALTER TABLE `buyingstores`
  ADD PRIMARY KEY (`id`);


ALTER TABLE `buyingstore_items`
  ADD PRIMARY KEY (`buyingstore_id`,`index`);


ALTER TABLE `cart_inventory`
  ADD PRIMARY KEY (`id`),
  ADD KEY `char_id` (`char_id`);


ALTER TABLE `char`
  ADD PRIMARY KEY (`char_id`),
  ADD UNIQUE KEY `name_key` (`name`),
  ADD KEY `account_id` (`account_id`),
  ADD KEY `party_id` (`party_id`),
  ADD KEY `guild_id` (`guild_id`),
  ADD KEY `online` (`online`);


ALTER TABLE `charlog`
  ADD PRIMARY KEY (`id`),
  ADD KEY `account_id` (`account_id`);


ALTER TABLE `char_reg_num`
  ADD PRIMARY KEY (`char_id`,`key`,`index`),
  ADD KEY `char_id` (`char_id`);


ALTER TABLE `char_reg_str`
  ADD PRIMARY KEY (`char_id`,`key`,`index`),
  ADD KEY `char_id` (`char_id`);


ALTER TABLE `clan`
  ADD PRIMARY KEY (`clan_id`);


ALTER TABLE `clan_alliance`
  ADD PRIMARY KEY (`clan_id`,`alliance_id`),
  ADD KEY `alliance_id` (`alliance_id`);


ALTER TABLE `db_roulette`
  ADD PRIMARY KEY (`index`);


ALTER TABLE `elemental`
  ADD PRIMARY KEY (`ele_id`);


ALTER TABLE `friends`
  ADD PRIMARY KEY (`char_id`,`friend_id`);


ALTER TABLE `global_acc_reg_num`
  ADD PRIMARY KEY (`account_id`,`key`,`index`),
  ADD KEY `account_id` (`account_id`);


ALTER TABLE `global_acc_reg_str`
  ADD PRIMARY KEY (`account_id`,`key`,`index`),
  ADD KEY `account_id` (`account_id`);


ALTER TABLE `guild`
  ADD PRIMARY KEY (`guild_id`,`char_id`),
  ADD UNIQUE KEY `guild_id` (`guild_id`),
  ADD KEY `char_id` (`char_id`);


ALTER TABLE `guild_alliance`
  ADD PRIMARY KEY (`guild_id`,`alliance_id`),
  ADD KEY `alliance_id` (`alliance_id`);


ALTER TABLE `guild_castle`
  ADD PRIMARY KEY (`castle_id`),
  ADD KEY `guild_id` (`guild_id`);


ALTER TABLE `guild_expulsion`
  ADD PRIMARY KEY (`guild_id`,`name`);


ALTER TABLE `guild_member`
  ADD PRIMARY KEY (`guild_id`,`char_id`),
  ADD KEY `char_id` (`char_id`);


ALTER TABLE `guild_position`
  ADD PRIMARY KEY (`guild_id`,`position`);


ALTER TABLE `guild_skill`
  ADD PRIMARY KEY (`guild_id`,`id`);


ALTER TABLE `guild_storage`
  ADD PRIMARY KEY (`id`),
  ADD KEY `guild_id` (`guild_id`);


ALTER TABLE `guild_storage_log`
  ADD PRIMARY KEY (`id`),
  ADD KEY `guild_id` (`guild_id`);


ALTER TABLE `homunculus`
  ADD PRIMARY KEY (`homun_id`);


ALTER TABLE `hotkey`
  ADD PRIMARY KEY (`char_id`,`hotkey`);


ALTER TABLE `interlog`
  ADD PRIMARY KEY (`id`),
  ADD KEY `time` (`time`);


ALTER TABLE `inventory`
  ADD PRIMARY KEY (`id`),
  ADD KEY `char_id` (`char_id`);


ALTER TABLE `ipbanlist`
  ADD PRIMARY KEY (`list`,`btime`);


ALTER TABLE `item_db2_re`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `UniqueAegisName` (`name_aegis`);


ALTER TABLE `item_db_re`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `UniqueAegisName` (`name_aegis`);


ALTER TABLE `login`
  ADD PRIMARY KEY (`account_id`),
  ADD UNIQUE KEY `web_auth_token_key` (`web_auth_token`),
  ADD KEY `name` (`userid`);


ALTER TABLE `mail`
  ADD PRIMARY KEY (`id`);


ALTER TABLE `mail_attachments`
  ADD PRIMARY KEY (`id`,`index`);


ALTER TABLE `mapreg`
  ADD PRIMARY KEY (`varname`,`index`);


ALTER TABLE `market`
  ADD PRIMARY KEY (`name`,`nameid`);


ALTER TABLE `memo`
  ADD PRIMARY KEY (`memo_id`),
  ADD KEY `char_id` (`char_id`);


ALTER TABLE `mercenary`
  ADD PRIMARY KEY (`mer_id`);


ALTER TABLE `mercenary_owner`
  ADD PRIMARY KEY (`char_id`);


ALTER TABLE `mob_db2_re`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `name_aegis` (`name_aegis`);


ALTER TABLE `mob_db_re`
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `name_aegis` (`name_aegis`);


ALTER TABLE `party`
  ADD PRIMARY KEY (`party_id`);


ALTER TABLE `pet`
  ADD PRIMARY KEY (`pet_id`);


ALTER TABLE `quest`
  ADD PRIMARY KEY (`char_id`,`quest_id`);


ALTER TABLE `sales`
  ADD PRIMARY KEY (`nameid`);


ALTER TABLE `sc_data`
  ADD PRIMARY KEY (`char_id`,`type`);


ALTER TABLE `skill`
  ADD PRIMARY KEY (`char_id`,`id`);


ALTER TABLE `skillcooldown`
  ADD PRIMARY KEY (`char_id`,`skill`);


ALTER TABLE `skill_homunculus`
  ADD PRIMARY KEY (`homun_id`,`id`);


ALTER TABLE `storage`
  ADD PRIMARY KEY (`id`),
  ADD KEY `account_id` (`account_id`);


ALTER TABLE `vendings`
  ADD PRIMARY KEY (`id`);


ALTER TABLE `vending_items`
  ADD PRIMARY KEY (`vending_id`,`index`);


ALTER TABLE `auction`
  MODIFY `auction_id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `cart_inventory`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;


ALTER TABLE `char`
  MODIFY `char_id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `charlog`
  MODIFY `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `clan`
  MODIFY `clan_id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `elemental`
  MODIFY `ele_id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `guild`
  MODIFY `guild_id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `guild_storage`
  MODIFY `id` int(10) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `guild_storage_log`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;


ALTER TABLE `homunculus`
  MODIFY `homun_id` int(11) NOT NULL AUTO_INCREMENT;


ALTER TABLE `interlog`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;


ALTER TABLE `inventory`
  MODIFY `id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `login`
  MODIFY `account_id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT, AUTO_INCREMENT=2000002;


ALTER TABLE `mail`
  MODIFY `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `mail_attachments`
  MODIFY `id` bigint(20) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `memo`
  MODIFY `memo_id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `mercenary`
  MODIFY `mer_id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `party`
  MODIFY `party_id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `pet`
  MODIFY `pet_id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `storage`
  MODIFY `id` int(11) UNSIGNED NOT NULL AUTO_INCREMENT;
COMMIT;

