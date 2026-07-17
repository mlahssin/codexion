# include "codexion.h"

bool is_empty(t_dongle *dongle)
{
    return dongle->size == 0;
}

void    swap(t_waiter *a, t_waiter *b)
{
    t_waiter tmp;
    tmp = *a;
    *a = *b;
    *b = tmp; 
}

void    buble_up(t_dongle *dongle, int index)
{
    int i = index;
    int parent;
    while (i > 0)
    {
        parent = (i - 1) / 2;
        if (dongle->waiters[i].prioroty <= dongle->waiters[parent].prioroty)
        {
            if (dongle->waiters[i].prioroty == dongle->waiters[parent].prioroty)
            {
                if (dongle->waiters[parent].coder_id < dongle->waiters[i].coder_id)
                    break;
            }
            swap(&dongle->waiters[i], &dongle->waiters[parent]);
            i = parent;
        }
        else
            break;
    }
}

int find_smallest(t_dongle   *dongle, int parent, int left_child, int right_child)
{
    int smallest;

    smallest = parent;
    if(left_child < dongle->size && dongle->waiters[left_child].prioroty <= dongle->waiters[smallest].prioroty)
    {
        if (dongle->waiters[left_child].prioroty == dongle->waiters[smallest].prioroty)
        {
            if (dongle->waiters[left_child].coder_id < dongle->waiters[smallest].coder_id)
                smallest = left_child;
        }
        else
            smallest = left_child;
    }
    if(right_child < dongle->size && dongle->waiters[right_child].prioroty <= dongle->waiters[smallest].prioroty)
    {
        if (dongle->waiters[right_child].prioroty == dongle->waiters[smallest].prioroty)
        {
            if (dongle->waiters[right_child].coder_id < dongle->waiters[smallest].coder_id)
                smallest = right_child;
        }
        else
            smallest = right_child;
    }
    return smallest;

}

void    shift_down(t_dongle *dongle)
{
    int i;
    int left_child;
    int right_child;
    int smallest;
    
    i = 0;
    while(i < dongle->size)
    {
        left_child = 2 * i + 1;
        right_child = 2 * i + 2;
        smallest = find_smallest(dongle, i, left_child, right_child);
        if(smallest == i)
            break;
        swap(&dongle->waiters[i], &dongle->waiters[smallest]);
        i = smallest;
    }
}