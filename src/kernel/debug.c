#include <debug.h>
#include <stdio.h>
#include <proc/process.h>
#include <proc/lock.h>

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

    if(PROCESS_isMultitaskingEnabled())
        acquire_mutex(&DEBUG_mutex);

    fputs(g_LogSeverityColors[level], VFS_FD_DEBUG);    // set color depending on level
    fprintf(VFS_FD_DEBUG, "[%s] ", module);             // write module
    vfprintf(VFS_FD_DEBUG, fmt, args);                  // write text
    fputs(g_ColorReset, VFS_FD_DEBUG);                  // reset format
    fputc('\n', VFS_FD_DEBUG);                          // newline

    if(PROCESS_isMultitaskingEnabled())
        release_mutex(&DEBUG_mutex);

    va_end(args);  
}