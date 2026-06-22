//
// Created by Douglas Bercot on 6/17/26.
//
#pragma once

#if !defined(RPU_DEBUG_MESSAGES)
#define RPU_DEBUG_MESSAGES 0
#define RPU_DEBUG_MESSAGE(msg)
#define RPU_DEBUG_DELAY(ms)
#define RPU_DEBUG_PRINTF(...)
#else
#define RPU_DEBUG_MESSAGE(msg) Serial.write(msg);
#define RPU_DEBUG_DELAY(ms) delay(ms)
#define RPU_DEBUG_PRINTF(...)           \
{                                 \
char _debug_buf[128];             \
sprintf(_debug_buf, __VA_ARGS__); \
Serial.write(_debug_buf);         \
}
#endif

