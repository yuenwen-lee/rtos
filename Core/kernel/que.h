/*
 * que.h
 *
 *  Created on: Aug 7, 2013
 *      Author: wayne
 */

#ifndef _QUE_H_
#define _QUE_H_

#define QUE_USE_INLINE


typedef struct que_s {
	struct que_s *next;
	struct que_s *prev;
} que_t __attribute__((aligned(4)));



static inline int que_is_empty(que_t *q_head)
{
	if (q_head->next == q_head && q_head->prev == q_head)
		return 1;  // it is an empty queue
	else
		return 0;
}

static inline int que_not_empty(que_t *q_head)
{
	if (q_head->next != q_head && q_head->prev != q_head)
		return 1;  // it is not an empty queue
	else
		return 0;
}

static inline int que_is_not_linked(que_t *que_p)
{
	if (que_p->next == que_p && que_p->prev == que_p)
		return 1;  // the queue is NOT linked
	else
		return 0;
}

#ifdef QUE_USE_INLINE  // replaced by in-line code .............................

// Make the q_elem self-pointed
static inline void que_init(que_t *q_elem)
{
	q_elem->next = q_elem->prev = q_elem;
	return;
}

// append the q_elem to the tail of q_head
static inline que_t *que_enque(que_t *q_head, que_t *q_elem)
{
	que_t *q_last;   // last queue element (just before q_head)

	q_last = q_head->prev;

	q_elem->next = q_head;
	q_elem->prev = q_last;

	q_last->next = q_elem;
	q_head->prev = q_elem;

	return q_elem;
}

// Remove the 1t queue element after the q_head
static inline que_t *que_deque(que_t *q_head)
{
	que_t *q_frst;   // 1st queue element (just after q_head)
	que_t *q_scnd;   // 2nd queue element (just after q_frst)

	if (q_head->next == q_head && q_head->prev == q_head)
		return 0;  // it is an empty queue

	q_frst = q_head->next;
	q_scnd = q_frst->next;

	q_head->next = q_scnd;
	q_scnd->prev = q_head;

	que_init(q_frst);  // clean up the removed queue

	return q_frst;
}

// Insert the q_elem after the anchor qeue, q_anch
static inline que_t *que_insert_after(que_t *q_anch, que_t *q_elem)
{
	que_t *q_next;   // the original queue after anchor queue

	q_next = q_anch->next;

	q_elem->next = q_next;
	q_elem->prev = q_anch;

	q_anch->next = q_elem;
	q_next->prev = q_elem;

	return q_elem;
}

// Insert the q_elem before the anchor qeue, q_anch
static inline que_t *que_insert_before(que_t *q_anch, que_t *q_elem)
{
	que_t *q_prev;   // the original queue after anchor queue

	q_prev = q_anch->prev;

	q_elem->next = q_anch;
	q_elem->prev = q_prev;

	q_prev->next = q_elem;
	q_anch->prev = q_elem;

	return q_elem;
}

static inline void que_remove(que_t *q_elem)
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

#else  // QUE_USE_INLINE replaced by in-line code .............................
void   que_init(que_t *q_elem);
que_t *que_enque(que_t *q_head, que_t *q_elem);
que_t *que_deque(que_t *q_head);
void   que_remove(que_t *q_elem);
que_t *que_insert_after(que_t *q_anch, que_t *q_elem);
que_t *que_insert_before(que_t *q_anch, que_t *q_elem);
int    que_check(que_t *q_head);

#endif // QUE_USE_INLINE

int que_check(que_t *q_head);


#endif /* _QUE_H_ */
