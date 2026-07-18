/**************************************************************************
 * MachineEeprom.h
 *
 * EEPROM address layout for machine-level settings shared across all
 * rulesets. These addresses are NOT game-specific — do not put ruleset
 * addresses here.
 *
 * Layout (all uint8_t unless noted):
 *   100–120  Core machine settings + Original Trident high score (4 B)
 *   121–139  Machine settings continued (was unused; 121–123 added here)
 *   140–188  Trident 2020 ruleset scores (defined in Trident2020.h)
 *   164–185  Machine audits and coin counts (see below)
 *   189+     Original Trident ruleset settings (defined in Trident2020.h)
 **************************************************************************/

#pragma once

// --- Core machine settings (100–120) ---
constexpr int EEPROM_BALL_SAVE_BYTE          = 100;
constexpr int EEPROM_FREE_PLAY_BYTE          = 101;
constexpr int EEPROM_SOUND_SELECTOR_BYTE     = 102;
// 103 = EEPROM_SKILL_SHOT_BYTE           (Trident2020.h)
constexpr int EEPROM_TILT_WARNING_BYTE       = 104;
// 105 = EEPROM_AWARD_OVERRIDE_BYTE       (Trident2020.h)
// 106 = EEPROM_BALLS_OVERRIDE_BYTE       (Trident2020.h)
constexpr int EEPROM_TOURNAMENT_SCORING_BYTE = 107;
constexpr int EEPROM_MUSIC_VOLUME_BYTE       = 108;
constexpr int EEPROM_SFX_VOLUME_BYTE         = 109;
constexpr int EEPROM_SCROLLING_SCORES_BYTE   = 110;
constexpr int EEPROM_CALLOUTS_VOLUME_BYTE    = 111;
constexpr int EEPROM_MATCH_FEATURE_BYTE      = 112;
constexpr int EEPROM_DIM_LEVEL_BYTE          = 113;
// 114–116 = T2020-specific adjustments   (Trident2020.h)
// 117–120 = EEPROM_ORIGINAL_HIGHSCORE_BYTE (Trident2020.h, 4 bytes)

// --- Machine settings (121–139, formerly unused gap) ---
constexpr int EEPROM_MAXIMUM_CREDITS_BYTE    = 121;  // moved from 215
constexpr int EEPROM_HIGH_SCORE_REPLAY_BYTE  = 122;  // moved from 216
constexpr int EEPROM_ACTIVE_RULE_SET_BYTE    = 123;  // moved from 217

// --- Machine audits and coin counts (164–185) ---
constexpr int EEPROM_CREDITS_BYTE            = 164;
constexpr int EEPROM_TOTAL_PLAYS_BYTE        = 165;  // uint32_t (4 bytes)
constexpr int EEPROM_TOTAL_REPLAYS_BYTE      = 169;  // uint32_t (4 bytes)
// 173–176 = EEPROM_HISCORE_BEAT_BYTE        (Trident2020.h, T2020 ruleset)
constexpr int EEPROM_CHUTE_2_COINS_BYTE      = 177;  // uint32_t (4 bytes)
constexpr int EEPROM_CHUTE_1_COINS_BYTE      = 181;  // uint32_t (4 bytes)
constexpr int EEPROM_CHUTE_3_COINS_BYTE      = 185;  // uint32_t (4 bytes)
