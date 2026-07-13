#include "codexion.h"


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

int extract_min(t_dongle *dongle)
{
    if(!is_empty(dongle))
        return dongle->waiters[0].coder_id;
    return -1;
}

void    buble_up(t_dongle *dongle, int index)
{
    int i = index;
    int parent;
    while (i > 0)
    {
        parent = (i - 1) / 2;
        if (dongle->waiters[i].prioroty < dongle->waiters[parent].prioroty)
        {
            swap(&dongle->waiters[i], &dongle->waiters[parent]);
            i = parent;
        }
        else
            break;
    }
}

void    push(t_dongle   *dongle, t_waiter   w)
{
    //if fifo : 0
    // prioroty : come first : elapsed time
    //if edf: 1
    //priorority:
    //coder last compile start + time_burnout
    if (dongle->size < 2)
    {
        if (is_empty(dongle))
        {
            // printf("hello\n");
            dongle->waiters[0] = w;
            dongle->size++;
            return;            
        }
        dongle->waiters[dongle->size] = w;

        dongle->size++;
        int i = dongle->size - 1;
        buble_up(dongle, dongle->size - 1);
    }
    else
        return;
    // printf("%d\n", dongle->size);
}

void    shif_down(t_dongle *dongle)
{
    int i = 0;
    int left_child;
    int right_child;
    

    while(i < dongle->size)
    {
        left_child = 2 * i + 1;
        right_child = 2 * i + 2;
        int smallest = i;
        if(left_child < dongle->size && dongle->waiters[left_child].prioroty < dongle->waiters[smallest].prioroty)
        {
            smallest = left_child;
        }
        if(right_child < dongle->size && dongle->waiters[right_child].prioroty < dongle->waiters[smallest].prioroty)
        {
            smallest = right_child;
        }
        if(smallest == i)
            break;
        swap(&dongle->waiters[i], &dongle->waiters[smallest]);
    }
}

int pop(t_dongle  *dongle)
{
    if(is_empty(dongle))
        return -1;
    int value;
    if (dongle->size == 1)
    {
        value  = extract_min(dongle);
        dongle->size--;
        return value;
    }

    value = extract_min(dongle);
    dongle->waiters[0] = dongle->waiters[dongle->size - 1];
    dongle->size--;
    shif_down(dongle);
    return  value;
}

// int main()
// {
//     t_dongle  dongle;
//     // dongle.capacity = 5;
//     dongle.size = 0;
//     t_waiter    w1, w2, w3, w4, w5;
//     w1.prioroty = 0;
//     w1.coder_id = 1;
//     w2.prioroty = 7;
//     w2.coder_id = 2;
//     // w3.prioroty = 4;
//     // w4.prioroty = 5;
//     // w5.prioroty = 0;

//     push(&dongle,w1);
//     printf("%d\n", extract_min(&dongle));
//     push(&dongle,w2);
//     printf("%d\n", extract_min(&dongle));

//     // push(&dongle,w3);
//     // printf("%d\n", extract_min(&dongle));
//     // push(&dongle,w4);
//     // printf("%d\n", extract_min(&dongle));
//     // push(&dongle,w5);
//     // printf("%d\n", extract_min(&dongle));
//     pop(&dongle);
//     printf("%d\n", extract_min(&dongle));
//     // pop(&dongle);
//     // pop(&dongle);
//     pop(&dongle);
//     printf("%d\n", extract_min(&dongle));
// }
