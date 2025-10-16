/*
 * Copyright (C) 2025,  Novice
 *
 * This file is part of the Novix software.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>

//============================================================================
//    INTERFACE DEFINITIONS / ENUMERATIONS / SIMPLE TYPEDEFS
//============================================================================

#define MIN_LOG_LEVEL LVL_DEBUG

typedef enum {
    LVL_DEBUG = 0,
    LVL_INFO = 1,
    LVL_WARN = 2,
    LVL_ERROR = 3,
    LVL_CRITICAL = 4
} DebugLevel;

//============================================================================
//    INTERFACE FUNCTION PROTOTYPES
//============================================================================

void logf(const char* module, DebugLevel level, const char* fmt, ...);
#define log_debug(module, ...) logf(module, LVL_DEBUG, __VA_ARGS__)
#define log_info(module, ...) logf(module, LVL_INFO, __VA_ARGS__)
#define log_warn(module, ...) logf(module, LVL_WARN, __VA_ARGS__)
#define log_err(module, ...) logf(module, LVL_ERROR, __VA_ARGS__)
#define log_crit(module, ...) logf(module, LVL_CRITICAL, __VA_ARGS__)