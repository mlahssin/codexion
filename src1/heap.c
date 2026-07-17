#include "codexion.h"

int extract_min(t_dongle *dongle)
{
    if(!is_empty(dongle))
        return dongle->waiters[0].coder_id;
    return -1;
}

void    push(t_dongle   *dongle, t_waiter   w)
{
    if (dongle->size < 2)
    {
        if (is_empty(dongle))
        {
            dongle->waiters[0] = w;
            dongle->size++;
            return;            
        }
        dongle->waiters[dongle->size] = w;

        dongle->size++;
        buble_up(dongle, dongle->size - 1);
    }
    else
        return;
}

int pop(t_dongle  *dongle)
{
    int value;

    if(is_empty(dongle))
        return -1;

    if (dongle->size == 1)
    {
        value  = extract_min(dongle);
        dongle->size--;
        return value;
    }

    value = extract_min(dongle);
    dongle->waiters[0] = dongle->waiters[dongle->size - 1];
    dongle->size--;
    shift_down(dongle);
    return  value;
}

