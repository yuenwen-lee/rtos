/*
 * ring_buf_event.h
 *
 *  Created on: Jan 4, 2017
 *      Author: Y.W Lee
 */ 

#ifndef _RING_BUF_EVENT_H_
#define _RING_BUF_EVENT_H_


typedef struct ring_buf_event_s {
    // basic info .......
    char      *buf_p;         // buffer address
    uint16_t   buf_len;       // buffer length
    uint16_t   offset_rd;     // read offset from buf_p
    uint16_t   offset_wr;     // write offset from buf_p
    // buf_seg[] write info
    uint8_t    seg_data_len;  // length of buf_seg
    uint8_t    seg_data_left; // free space left in buf_seg[]
    char      *buf_seg_p;     // write pointer in buf_seg[]
    // statistic ........
    uint32_t   event_send;    // number of total event dispatched
    uint32_t   event_ack;     // 
    uint32_t   byte_send;
    uint32_t   byte_ack;
    uint32_t   enevt_wrap;
    uint32_t   enevt_wrap_ack;
    uint32_t   err_ovflow;
    sem_obj_t  sem;
} ring_buf_event_t;


void ring_buf_event_init(ring_buf_event_t *event_p, char *buf_p, uint32_t buf_len);

void *ring_buf_event_seg_get(ring_buf_event_t *event_p, uint32_t size_req);
void ring_buf_event_seg_free(ring_buf_event_t *event_p);

void ring_buf_event_post(ring_buf_event_t *event_p);
void ring_buf_event_wait(ring_buf_event_t *event_p, char **buf_p, uint32_t *buf_len_p);

void ring_buf_event_dump(ring_buf_event_t *event_p);
void ring_buf_event_buf_dump(ring_buf_event_t *event_p);


static inline uint32_t ring_buf_event_seg_data_left(ring_buf_event_t *event_p)
{
    return ((uint32_t) event_p->seg_data_left);
}

static inline uint32_t ring_buf_event_seg_data_full(ring_buf_event_t *event_p)
{
    return (event_p->seg_data_left == 0);
}

static inline uint32_t ring_buf_event_seg_data_empty(ring_buf_event_t *event_p)
{
    return (event_p->seg_data_left == event_p->seg_data_len);
}

static inline uint32_t ring_buf_event_write_char(ring_buf_event_t *event_p, char c)
{
    *event_p->buf_seg_p++ = c;
    event_p->seg_data_left--;
    return ((uint32_t) event_p->seg_data_left);
}

static inline uint32_t ring_buf_event_clear_char(ring_buf_event_t *event_p)
{
    event_p->buf_seg_p--;
    event_p->seg_data_left++;
    return ((uint32_t) event_p->seg_data_left);
}

static inline uint32_t ring_buf_event_update(ring_buf_event_t *event_p, uint32_t len)
{
    event_p->buf_seg_p += len;
    event_p->seg_data_left -= len;
    return ((uint32_t) event_p->seg_data_left);
}


#endif /* _RING_BUF_EVENT_H_ */
