// #include "codexion2.h"




// bool is_empty(t_heap *heap)
// {
//     return heap->size == 0;
// }

// void    swap(t_waiter *a, t_waiter *b)
// {
//     t_waiter tmp;
//     tmp = *a;
//     *a = *b;
//     *b = tmp; 
// }

// t_waiter *extract_min(t_heap *heap)
// {
//     if(!is_empty(heap))
//         return heap->waiters;
//     return NULL;
// }

// void    buble_up(t_heap *heap, int index)
// {
//     int i = index;
//     while (i > 0)
//     {
//         if (heap->waiters[i].prioroty < heap->waiters[(i - 1) / 2].prioroty)
//         {
//             swap(&heap->waiters[i], &heap->waiters[(i - 1) / 2]);
//             i = (i - 1) / 2;
//         }
//         else
//             break;
//     }
// }

// void    push(t_heap   *heap, t_waiter   w)
// {
//     //if fifo : 0
//     // prioroty : come first : elapsed time
//     //if edf: 1
//     //priorority:
//     //coder last compile start + time_burnout
//     if (heap->size < heap->capacity)
//     {
//         if (is_empty(heap))
//         {
//                 heap->waiters[0] = w;
//                 heap->size++;
//                 return;            
//         }
//         heap->waiters[heap->size] = w;
//         heap->size++;
//         int i = heap->size - 1;
//         buble_up(heap, heap->size - 1);
//     }
//     else
//         return;
// }

// void    shif_down(t_heap *heap)
// {
//     int i = 0;
//     int left_child;
//     int right_child;
    

//     while(i < heap->size)
//     {
//         left_child = 2 * i + 1;
//         right_child = 2 * i + 2;
//         int smallest = i;
//         if(heap->waiters[left_child].prioroty < heap->waiters[smallest].prioroty)
//         {
//             smallest = left_child;
//         }
//         if(heap->waiters[right_child].prioroty < heap->waiters[smallest].prioroty)
//         {
//             smallest = right_child;
//         }
//         if(smallest == i)
//             break;
//         swap(&heap->waiters[i], &heap->waiters[smallest]);
//     }
// }
// t_waiter *pop(t_heap  *heap)
// {
//     if(is_empty(heap))
//     {
//         return NULL;
//     }
//     if (heap->size == 1)
//     {
//        t_waiter *w = extract_min(heap);
//         heap->size--;
//         return w;
//     }

//     t_waiter *w = extract_min(heap);
//     heap->waiters[0] = heap->waiters[heap->size - 1];
//     heap->size--;
//     shif_down(heap);
//     return  w;
// }


// // int main()
// // {
// //     t_heap  heap;
// //     heap.capacity = 5;
// //     heap.size = 0;
// //     heap.waiters = malloc(sizeof(t_waiter) * heap.capacity);
// //     t_waiter    w1, w2, w3, w4, w5;
// //     w1.prioroty = 10;
// //     w2.prioroty = 7;
// //     w3.prioroty = 4;
// //     w4.prioroty = 5;
// //     w5.prioroty = 0;

// //     push(&heap,w1);
// //     printf("%d\n", extract_min(&heap)->prioroty);
// //     push(&heap,w2);
// //     printf("%d\n", extract_min(&heap)->prioroty);
// //     push(&heap,w3);
// //     printf("%d\n", extract_min(&heap)->prioroty);
// //     push(&heap,w4);
// //     printf("%d\n", extract_min(&heap)->prioroty);
// //     push(&heap,w5);
// //     printf("%d\n", extract_min(&heap)->prioroty);
// //     pop(&heap);
// //     printf("%d\n", extract_min(&heap)->prioroty);
// // }
