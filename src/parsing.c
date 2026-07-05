# include "codexion.h"

static long parse_args(char *str)
{
    int i = 0;
    long num = 0;

    if (!str ||str[0] == '\0')
        return (-1);
    
    while (str[i])
    {
        if (str[i] < '0' || str[i] > '9')
            return (-1);
            
        num = num * 10 + (str[i] - '0');
        
        if (num > INT_MAX)
            return (-1);
        i++;
    }

    return (num);
}


static void fill_args(t_params *p, char **av)
{
    p->num_coders       = parse_args(av[1]);
    p->time_to_burnout  = parse_args(av[2]);
    p->time_to_compile  = parse_args(av[3]);
    p->time_to_debug    = parse_args(av[4]);
    p->time_to_refactor = parse_args(av[5]);
    p->num_compiles     = parse_args(av[6]);
    p->cooldown         = parse_args(av[7]);
}



int parse(int ac, char **av, t_params *p)
{
    if (ac != 9)
        return (0);
    fill_args(p, av);
    if (p->num_coders <= 0)
        return (0);
    if (p->time_to_burnout == -1)
        return (0);
    if (p->time_to_compile == -1)
        return (0);
    if (p->time_to_debug == -1)
        return (0);
    if (p->time_to_refactor == -1)
        return (0);
    if (p->num_compiles == -1)
        return (0);
    if (p->cooldown == -1)
        return (0);
    if (strcmp(av[8], "fifo") != 0 && strcmp(av[8], "edf") != 0)
        return (0);
    if (strcmp(av[8], "fifo") == 0)
        p->scheduler_type = 0;
    else
        p->scheduler_type = 1;
    return (1);
}


