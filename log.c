static int log_lvl = LOG_ERR;

void Log(int lvl, const char* fmt, ...)
{
    static const char* LOG_LVL_NAME[] = {
        [LOG_EMERG]   = "[emerg]",
        [LOG_ALERT]   = "[alert]",
        [LOG_CRIT]    = " [crit]",
        [LOG_ERR]     = "[error]",
        [LOG_WARNING] = " [warn]",
        [LOG_NOTICE]  = " [noti]",
        [LOG_INFO]    = " [info]",
        [LOG_DEBUG]   = "[debug]"
    };

    if (lvl > log_lvl) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    if (log_to_stderr) {
        struct tm tm;
        time_t t;
        time(&t);
        localtime_r(&t, &tm);

        char buff[2048];
        size_t idx = 0;
        idx += strftime(buff + idx, sizeof(buff) - idx, "%Y-%m-%d %T", &tm);
        idx += snprintf(buff + idx, sizeof(buff) - idx, " %s ", LOG_LVL_NAME[lvl]);
        idx += vsnprintf(buff + idx, sizeof(buff) - idx, fmt, ap);

        fwrite(buff, 1, idx, stderr);
        if (buff[idx - 1] != '\n') {
            fputc('\n', stderr);
        }
    } else {
        vsyslog(lvl, fmt, ap);
    }
    va_end(ap);
}
