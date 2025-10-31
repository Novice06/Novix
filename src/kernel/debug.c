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

#include <debug.h>
#include <stdio.h>

#include <multitasking/scheduler.h>
#include <multitasking/lock.h>

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

static const char* const g_LogSeverityColors[] =
{
    [LVL_DEBUG]        = "\033[2;37m",
    [LVL_INFO]         = "\033[37m",
    [LVL_WARN]         = "\033[1;33m",
    [LVL_ERROR]        = "\033[1;31m",
    [LVL_CRITICAL]     = "\033[1;37;41m",
};

static const char* const g_ColorReset = "\033[0m";
mutex_t DEBUG_mutex;

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

void logf(const char* module, DebugLevel level, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    if (level < MIN_LOG_LEVEL)
        return;

    if(is_schedulerEnabled())
        acquire_mutex(&DEBUG_mutex);

    fputs(g_LogSeverityColors[level], VFS_FD_DEBUG);    // set color depending on level
    fprintf(VFS_FD_DEBUG, "[%s] ", module);             // write module
    vfprintf(VFS_FD_DEBUG, fmt, args);                  // write text
    fputs(g_ColorReset, VFS_FD_DEBUG);                  // reset format
    fputc('\n', VFS_FD_DEBUG);                          // newline

    if(is_schedulerEnabled())
        release_mutex(&DEBUG_mutex);

    va_end(args);  
}