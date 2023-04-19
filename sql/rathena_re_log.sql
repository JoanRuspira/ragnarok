

SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
START TRANSACTION;
SET time_zone = "+00:00";


CREATE TABLE `atcommandlog` (
  `atcommand_id` mediumint(9) UNSIGNED NOT NULL,
  `atcommand_date` datetime NOT NULL,
  `account_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `char_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `char_name` varchar(25) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `map` varchar(11) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `command` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


CREATE TABLE `branchlog` (
  `branch_id` mediumint(9) UNSIGNED NOT NULL,
  `branch_date` datetime NOT NULL,
  `account_id` int(11) NOT NULL DEFAULT 0,
  `char_id` int(11) NOT NULL DEFAULT 0,
  `char_name` varchar(25) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `map` varchar(11) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `cashlog` (
  `id` int(11) NOT NULL,
  `time` datetime NOT NULL,
  `char_id` int(11) NOT NULL DEFAULT 0,
  `type` enum('T','V','P','M','S','N','D','C','A','E','I','B','$') COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'S',
  `cash_type` enum('O','K','C') COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'O',
  `amount` int(11) NOT NULL DEFAULT 0,
  `map` varchar(11) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `chatlog` (
  `id` bigint(20) NOT NULL,
  `time` datetime NOT NULL,
  `type` enum('O','W','P','G','M','C') COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'O',
  `type_id` int(11) NOT NULL DEFAULT 0,
  `src_charid` int(11) NOT NULL DEFAULT 0,
  `src_accountid` int(11) NOT NULL DEFAULT 0,
  `src_map` varchar(11) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `src_map_x` smallint(4) NOT NULL DEFAULT 0,
  `src_map_y` smallint(4) NOT NULL DEFAULT 0,
  `dst_charname` varchar(25) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `message` varchar(150) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `feedinglog` (
  `id` int(11) NOT NULL,
  `time` datetime NOT NULL,
  `char_id` int(11) NOT NULL,
  `target_id` int(11) NOT NULL,
  `target_class` smallint(11) NOT NULL,
  `type` enum('P','H','O') COLLATE utf8mb4_unicode_ci NOT NULL,
  `intimacy` int(11) UNSIGNED NOT NULL,
  `item_id` int(10) UNSIGNED NOT NULL,
  `map` varchar(11) COLLATE utf8mb4_unicode_ci NOT NULL,
  `x` smallint(5) UNSIGNED NOT NULL,
  `y` smallint(5) UNSIGNED NOT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `loginlog` (
  `time` datetime NOT NULL,
  `ip` varchar(15) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `user` varchar(23) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `rcode` tinyint(4) NOT NULL DEFAULT 0,
  `log` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `mvplog` (
  `mvp_id` mediumint(9) UNSIGNED NOT NULL,
  `mvp_date` datetime NOT NULL,
  `kill_char_id` int(11) NOT NULL DEFAULT 0,
  `monster_id` smallint(6) NOT NULL DEFAULT 0,
  `prize` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `mvpexp` bigint(20) UNSIGNED NOT NULL DEFAULT 0,
  `map` varchar(11) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `npclog` (
  `npc_id` mediumint(9) UNSIGNED NOT NULL,
  `npc_date` datetime NOT NULL,
  `account_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `char_id` int(11) UNSIGNED NOT NULL DEFAULT 0,
  `char_name` varchar(25) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `map` varchar(11) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `mes` varchar(255) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `picklog` (
  `id` int(11) NOT NULL,
  `time` datetime NOT NULL,
  `char_id` int(11) NOT NULL DEFAULT 0,
  `type` enum('M','P','L','T','V','S','N','C','A','R','G','E','B','O','I','X','D','U','$','F','Y','Z','Q','H') COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'P',
  `nameid` int(10) UNSIGNED NOT NULL DEFAULT 0,
  `amount` int(11) NOT NULL DEFAULT 1,
  `refine` tinyint(3) UNSIGNED NOT NULL DEFAULT 0,
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
  `map` varchar(11) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT '',
  `bound` tinyint(1) UNSIGNED NOT NULL DEFAULT 0,
  `enchantgrade` tinyint(3) UNSIGNED NOT NULL DEFAULT 0
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;



CREATE TABLE `zenylog` (
  `id` int(11) NOT NULL,
  `time` datetime NOT NULL,
  `char_id` int(11) NOT NULL DEFAULT 0,
  `src_id` int(11) NOT NULL DEFAULT 0,
  `type` enum('T','V','P','M','S','N','D','C','A','E','I','B','K') COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT 'S',
  `amount` int(11) NOT NULL DEFAULT 0,
  `map` varchar(11) COLLATE utf8mb4_unicode_ci NOT NULL DEFAULT ''
) ENGINE=MyISAM DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;


ALTER TABLE `atcommandlog`
  ADD PRIMARY KEY (`atcommand_id`),
  ADD KEY `account_id` (`account_id`),
  ADD KEY `char_id` (`char_id`);


ALTER TABLE `branchlog`
  ADD PRIMARY KEY (`branch_id`),
  ADD KEY `account_id` (`account_id`),
  ADD KEY `char_id` (`char_id`);


ALTER TABLE `cashlog`
  ADD PRIMARY KEY (`id`),
  ADD KEY `type` (`type`);


ALTER TABLE `chatlog`
  ADD PRIMARY KEY (`id`),
  ADD KEY `src_accountid` (`src_accountid`),
  ADD KEY `src_charid` (`src_charid`);


ALTER TABLE `feedinglog`
  ADD PRIMARY KEY (`id`);


ALTER TABLE `loginlog`
  ADD KEY `ip` (`ip`);


ALTER TABLE `mvplog`
  ADD PRIMARY KEY (`mvp_id`);


ALTER TABLE `npclog`
  ADD PRIMARY KEY (`npc_id`),
  ADD KEY `account_id` (`account_id`),
  ADD KEY `char_id` (`char_id`);


ALTER TABLE `picklog`
  ADD PRIMARY KEY (`id`),
  ADD KEY `type` (`type`);


ALTER TABLE `zenylog`
  ADD PRIMARY KEY (`id`),
  ADD KEY `type` (`type`);


ALTER TABLE `atcommandlog`
  MODIFY `atcommand_id` mediumint(9) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `branchlog`
  MODIFY `branch_id` mediumint(9) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `cashlog`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;


ALTER TABLE `chatlog`
  MODIFY `id` bigint(20) NOT NULL AUTO_INCREMENT;


ALTER TABLE `feedinglog`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;


ALTER TABLE `mvplog`
  MODIFY `mvp_id` mediumint(9) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `npclog`
  MODIFY `npc_id` mediumint(9) UNSIGNED NOT NULL AUTO_INCREMENT;


ALTER TABLE `picklog`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;


ALTER TABLE `zenylog`
  MODIFY `id` int(11) NOT NULL AUTO_INCREMENT;
COMMIT;
