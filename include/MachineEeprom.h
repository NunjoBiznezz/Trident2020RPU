/**************************************************************************
 * MachineEeprom.h
 *
 * Single source of truth for all EEPROM address assignments.
 * Sections: Machine (shared), Original Trident, Trident 2020.
 **************************************************************************/

#pragma once
#include <stdint.h>

// ---------------------------------------------------------------------------
// Version — stored at byte 0; mismatch resets all settings to defaults.
// Bump EEPROM_VERSION on every build.
// ---------------------------------------------------------------------------
constexpr int     EEPROM_VERSION_BYTE   = 0;
constexpr uint8_t EEPROM_VERSION        = 1;  // *** bump on every build ***

// Range of addresses erased on a version mismatch.
constexpr int EEPROM_SETTINGS_START = 100;
constexpr int EEPROM_SETTINGS_END   = 221;

// ---------------------------------------------------------------------------
// Machine — shared across all rulesets
// ---------------------------------------------------------------------------
constexpr int EEPROM_FREE_PLAY_BYTE           = 101;  // uint8_t (bool)
constexpr int EEPROM_SOUND_SELECTOR_BYTE      = 102;  // uint8_t
constexpr int EEPROM_TILT_WARNING_BYTE        = 104;  // uint8_t
constexpr int EEPROM_TOURNAMENT_SCORING_BYTE  = 107;  // uint8_t (bool)
constexpr int EEPROM_MUSIC_VOLUME_BYTE        = 108;  // uint8_t  0–10
constexpr int EEPROM_SFX_VOLUME_BYTE          = 109;  // uint8_t  0–10
constexpr int EEPROM_SCROLLING_SCORES_BYTE    = 110;  // uint8_t (bool)
constexpr int EEPROM_CALLOUTS_VOLUME_BYTE     = 111;  // uint8_t  0–10
constexpr int EEPROM_MATCH_FEATURE_BYTE       = 112;  // uint8_t (bool)
constexpr int EEPROM_DIM_LEVEL_BYTE           = 113;  // uint8_t  2–3
constexpr int EEPROM_MAXIMUM_CREDITS_BYTE     = 121;  // uint8_t
constexpr int EEPROM_HIGH_SCORE_REPLAY_BYTE   = 122;  // uint8_t (bool)
constexpr int EEPROM_ACTIVE_RULE_SET_BYTE     = 123;  // uint8_t (RuleSet)
constexpr int EEPROM_CREDITS_BYTE             = 164;  // uint8_t
constexpr int EEPROM_TOTAL_PLAYS_BYTE         = 165;  // uint32_t (4 bytes)
constexpr int EEPROM_TOTAL_REPLAYS_BYTE       = 169;  // uint32_t (4 bytes)
constexpr int EEPROM_CHUTE_2_COINS_BYTE       = 177;  // uint32_t (4 bytes)
constexpr int EEPROM_CHUTE_1_COINS_BYTE       = 181;  // uint32_t (4 bytes)
constexpr int EEPROM_CHUTE_3_COINS_BYTE       = 185;  // uint32_t (4 bytes)

// ---------------------------------------------------------------------------
// Original Trident ruleset
// ---------------------------------------------------------------------------
constexpr int EEPROM_ORIGINAL_HIGHSCORE_BYTE            = 117;  // uint32_t (4 bytes, 117–120)
constexpr int EEPROM_ORIGINAL_AWARD_OVERRIDE_BYTE       = 189;  // uint8_t
constexpr int EEPROM_ORIGINAL_AWARD_SCORE_1_BYTE        = 190;  // uint32_t (4 bytes)
constexpr int EEPROM_ORIGINAL_AWARD_SCORE_2_BYTE        = 194;  // uint32_t (4 bytes)
constexpr int EEPROM_ORIGINAL_AWARD_SCORE_3_BYTE        = 198;  // uint32_t (4 bytes)
constexpr int EEPROM_ORIGINAL_EXTRA_BALL_SCORE_BYTE     = 202;  // uint32_t (4 bytes)
constexpr int EEPROM_ORIGINAL_SPECIAL_SCORE_BYTE        = 206;  // uint32_t (4 bytes)
constexpr int EEPROM_ORIGINAL_HISCORE_BEAT_BYTE         = 210;  // uint32_t (4 bytes)
constexpr int EEPROM_ORIGINAL_BALLS_OVERRIDE_BYTE       = 214;  // uint8_t
constexpr int EEPROM_ORIGINAL_DROP_TARGET_SPECIAL_BYTE  = 218;  // uint8_t  (4 or 5)
constexpr int EEPROM_ORIGINAL_HIGH_SCORE_FEATURE_BYTE   = 219;  // uint8_t  (0=extra ball, 1=replay)
constexpr int EEPROM_ORIGINAL_MELODY_OPTION_BYTE        = 220;  // uint8_t
constexpr int EEPROM_ORIGINAL_HSTD_FEATURE_BYTE         = 221;  // uint8_t

// ---------------------------------------------------------------------------
// Trident 2020 ruleset
// ---------------------------------------------------------------------------
constexpr int EEPROM_BALL_SAVE_BYTE                  = 100;  // uint8_t
constexpr int EEPROM_SKILL_SHOT_BYTE                 = 103;  // uint8_t
constexpr int EEPROM_AWARD_OVERRIDE_BYTE             = 105;  // uint8_t
constexpr int EEPROM_BALLS_OVERRIDE_BYTE             = 106;  // uint8_t
constexpr int EEPROM_SHARP_SHOOTER_START_BONUS_BYTE  = 114;  // uint8_t
constexpr int EEPROM_TARGET_SPECIAL_BONUS_BYTE       = 115;  // uint8_t
constexpr int EEPROM_STANDUP_SPECIAL_LEVEL_BYTE      = 116;  // uint8_t
constexpr int EEPROM_EXTRA_BALL_SCORE_BYTE           = 140;  // uint32_t (4 bytes)
constexpr int EEPROM_SPECIAL_SCORE_BYTE              = 144;  // uint32_t (4 bytes)
constexpr int EEPROM_AWARD_SCORE_1_BYTE              = 148;  // uint32_t (4 bytes)
constexpr int EEPROM_AWARD_SCORE_2_BYTE              = 152;  // uint32_t (4 bytes)
constexpr int EEPROM_AWARD_SCORE_3_BYTE              = 156;  // uint32_t (4 bytes)
constexpr int EEPROM_HIGHSCORE_BYTE                  = 160;  // uint32_t (4 bytes)
constexpr int EEPROM_HISCORE_BEAT_BYTE               = 173;  // uint32_t (4 bytes)
