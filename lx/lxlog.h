#ifndef _LX_LOG_H_
#define _LX_LOG_H_

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

namespace lx {

enum log_level {
    log_level_error,
    log_level_warn,
    log_level_noti,
    log_level_info,
    log_level_debug,
    log_level_trace,
};

class log {
public:
    void print (enum log_level level, const char *func, int line, const char *fmt, va_list ap);
    void _cprint (enum log_level level, const char *func, int line, const char *fmt, ...) {
        if (mask & (1U<<level))
        {
            va_list ap;
            va_start (ap, fmt);
            print (level, func, line, fmt, ap);
            va_end (ap);
        }
    }

    log (log const&) = delete;
    log (log&&) = delete;
    log& operator= (log const&) = delete;
    log& operator= (log &&) = delete;

    log (const char *name) {
        printf ("%s.%d log(%s)\n", __func__, __LINE__, name);
        this->name  = name;
        mask = 0xffffffff;
    }

protected:
    uint32_t mask;
    const char *name;
};

}

#define lxlog_define(name)		lx::log __log__##name(#name)
#define lx_error(name, fmt, args...)	__log__##name._cprint (lx::log_level_error, __func__, __LINE__, fmt, ##args)
#define lx_warn(name, fmt, args...)	__log__##name._cprint (lx::log_level_warn,  __func__, __LINE__, fmt, ##args)
#define lx_noti(name, fmt, args...)	__log__##name._cprint (lx::log_level_noti,  __func__, __LINE__, fmt, ##args)
#define lx_info(name, fmt, args...)	__log__##name._cprint (lx::log_level_info,  __func__, __LINE__, fmt, ##args)
#define lx_debug(name, fmt, args...)	__log__##name._cprint (lx::log_level_debug, __func__, __LINE__, fmt, ##args)
#define lx_trace(name, fmt, args...)	__log__##name._cprint (lx::log_level_trace, __func__, __LINE__, fmt, ##args)

#endif
