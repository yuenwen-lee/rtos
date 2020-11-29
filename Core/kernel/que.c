/*
 * que.c
 *
 *  Created on: Aug 7, 2013
 *      Author: Y.W. Lee
 */ 

#include <stdio.h>
#include <stdint.h>

#include "kernel/que.h"


#define MAX_WALK_COUNT  65536


#ifndef QUE_USE_INLINE  // replaced by in-line code

//
// Make the q_elem self-pointed
//
void que_init(que_t *q_elem)
{
    q_elem->next = q_elem->prev = q_elem;
    return;
}


//
// append the q_elem to the tail of q_head
//
que_t *que_enque(que_t *q_head, que_t *q_elem)
{
    que_t *q_last;   // last queue element (just before q_head)

    q_last = q_head->prev;

    q_elem->next = q_head;
    q_elem->prev = q_last;

    q_last->next = q_elem;
    q_head->prev = q_elem;

    return q_elem;
}


//
// Remove the 1t queue element after the q_head
//
que_t *que_deque(que_t *q_head)
{
    que_t *q_frst;   // 1st queue element (just after q_head)
    que_t *q_scnd;   // 2nd queue element (just after q_frst)

    if (q_head->next == q_head && q_head->prev == q_head)
        return NULL;  // it is an empty queue

    q_frst = q_head->next;
    q_scnd = q_frst->next;

    q_head->next = q_scnd;
    q_scnd->prev = q_head;

    que_init(q_frst);  // clean up the removed queue

    return q_frst;
}


//
// Insert the q_elem after the anchor qeue, q_anch
//
que_t *que_insert_after(que_t *q_anch, que_t *q_elem)
{
    que_t *q_next;   // the original queue after anchor queue

    q_next = q_anch->next;

    q_elem->next = q_next;
    q_elem->prev = q_anch;

    q_anch->next = q_elem;
    q_next->prev = q_elem;

    return q_elem;
}


//
// Insert the q_elem before the anchor qeue, q_anch
//
que_t *que_insert_before(que_t *q_anch, que_t *q_elem)
{
    que_t *q_prev;   // the original queue after anchor queue

    q_prev = q_anch->prev;

    q_elem->next = q_anch;
    q_elem->prev = q_prev;

    q_prev->next = q_elem;
    q_anch->prev = q_elem;

    return q_elem;
}


void que_remove(que_t *q_elem)
{
    que_t *q_next;
    que_t *q_prev;

    if (q_elem->next == q_elem && q_elem->prev == q_elem)
        return;  // the queue element is isolated

    q_prev = q_elem->prev;  // the one before q_elem
    q_next = q_elem->next;  // the one after  q_elem

    q_prev->next = q_next;
    q_next->prev = q_prev;

    que_init(q_elem);  // clean up
}

#endif // QUE_USE_INLINE


int que_check(que_t *q_head)
{
    que_t *q_elem;
    int    n;

    n = 0;
    q_elem = q_head;
    while (q_elem->next != q_head && n < MAX_WALK_COUNT) {
        if (q_elem->next->prev != q_elem)
            return -1;
        n++;
        q_elem = q_elem->next;
    }

    if (n <= MAX_WALK_COUNT)
        return n;
    else
        return -1;
}
