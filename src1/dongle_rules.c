# include "codexion.h"

bool is_dongle_available(t_dongle   *dongle)
{
    int value;
    value = 0;
    value = dongle->used;
    return value == -1;
}

bool    cooldown_finished(t_dongle   *dongle, long long start)
{
    return elapsed_time(start) >= dongle->released_at;
}
bool    is_only_cooldown_left(t_dongle *dongle, t_coder    *coder)
{
    return(extract_min(dongle) == coder->id && is_dongle_available(dongle) && !cooldown_finished(dongle, coder->shared->start));
}

bool    can_coder_take_dongle(t_dongle *dongle, t_coder    *coder)
{
    return(is_dongle_available(dongle) && extract_min(dongle) == coder->id && cooldown_finished(dongle, coder->shared->start));
}

