


//heap_is_empty check if heap has waiters



// heap_peek look at next waiter without removing



// heap push : add new waiter into the heap and repsitions it correctly
//steps : add new waiter to the end of the array
//shift up : buble it until heap ordered is restored


// typedef struct s_heap
// {
//     t_waiter    *data; //array of waiters stored as a tree
//     int         size; // how many waiters are currently in heap
//     int         capacity;// max waiter that can the heap hold
// }   t_heap;


// typedef struct s_waiter {
//     t_coder     *coder; // who is waiter --pointer to the coder
//     long long   priority;    // arrival timestamp (FIFO) or deadline (EDF)
//     int   tiebreak;    // coder->id for determinism lower id win
// } t_waiter;

//  heap pop : remove and return the most urgent waiter and restore the heap


# include "codexion.h"

// heap init : create the heap : it allocate memory for the heap and sets
// it up ready to use


void    heap_init(t_heap *heap, int capacity)
{
    t_waiter    *waiters;

    heap->size = 0;
    heap->data = NULL;
    heap->capacity = 0;
    waiters = malloc(sizeof(t_waiter) * capacity);

    if (! waiters)
        return;

    heap->size = 0;
    heap->data = waiters;
    heap->capacity = capacity;
}

int has_priority(t_waiter a, t_waiter b, int use_edf)
{
    // same comparison either way — smaller value wins
    // difference is what "priority" means:
    
    // FIFO (use_edf = 0):
    // priority = arrival timestamp
    // smaller timestamp = requested earlier = goes first

    // EDF (use_edf = 1):
    // priority = burnout deadline
    // smaller deadline = burns out sooner = goes first

    if (a.priority != b.prioritdy)
        return (a.priority < b.priority);
    return (a.tiebreak < b.tiebreak);
}

void    swap_waiters(t_waiter *a, t_waiter *b)
{
    t_waiter    tmp;

    tmp = *a;
    *a  = *b;
    *b  = tmp;
}


void heap_push(t_heap *heap, t_waiter waiter, int use_edf)
{
    int current;
    int parent;

    if (use_edf)
        waiter.priority = waiter.coder->last_compile_start + waiter.coder->sim->params.time_to_burnout;
    else
        waiter.priority = now_ms();

    waiter.tiebreak = waiter.coder->id;

    if (heap->size >= heap->capacity)
        return;

    heap->data[heap->size] = waiter;

    heap->size++;
    current = heap->size - 1;

    while (current > 0)
    {
        parent = (current - 1) / 2;
        
        if (has_priority(heap->data[current], heap->data[parent], use_edf))
        {
            swap_waiters(&heap->data[current], &heap->data[parent]);
            current = parent;
        }
        else
            break;
    } 
}

t_waiter    heap_pop(t_heap *h, int use_edf)
{
    t_waiter    save;
    int parent;

    int left_child;
    int right_child;

    if (use_edf)
        waiter.priority = waiter.coder->last_compile_start + waiter.coder->sim->params.time_to_burnout;
    else
        waiter.priority = now_ms();

    
    parent = 0;
    // save the root value
    save = h->data[0];
    // move last to root m shrink size
    h->data[0] = h->data[h->size - 1];
    h->size--;

    // sift down loop;
    while (parent < h->size)
    {
        parent = (current - 1) / 2;
        
        left_child = parent * 2 + 1;
        right_child = parent * 2 + 2;

        if (has_priority(heap->data[current], heap->data[parent], use_edf))
        {
            swap_waiters(&heap->data[current], &heap->data[parent]);
            current = parent;
        }
        else
            break;
    } 
    
    return save;

}




int heap_is_empty(t_heap *heap)
{
    if (heap->size == 0)
        return (1);
    return (0);
}

