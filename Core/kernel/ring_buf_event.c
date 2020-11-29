/*
 * ring_buf_event.c
 *
 *  Created on: Jan 4, 2017
 *      Author: Y.W Lee
 */ 

#include <stdio.h>
#include <stdint.h>

#include "kernel/task.h"
#include "kernel/sem.h"
#include "kernel/ring_buf_event.h"


#define DRV_BUF_LEN_MAX     65536  // NOTE: 'buf_len' is uint16_t
#define DRV_BUF_SEG_MAX      (254 + sizeof(ring_buf_seg_t))
#define DRV_BUF_SEG_MIN      (  8 + sizeof(ring_buf_seg_t))


typedef struct ring_buf_seg_ {
#define DRV_BUF_SEG_LEN_RESET     255
    uint8_t len;      // 0 .......... empty, no data
                      // 1 ~ 245 .... valid data
                      // 255 ........ reset read pointer (wrap around)
    uint8_t data[0];  // maximum 254 bytes long
} ring_buf_seg_t;


/* ********************************************************* */
/* ********************************************************* */

void ring_buf_event_init (ring_buf_event_t *event_p,
                         char *buf_p, uint32_t buf_len)
{
//  assert(buf_len > DRV_BUF_SEG_MIN)
//  assert(buf_len < DRV_BUF_LEN_MAX);

    event_p->buf_p = buf_p;
    event_p->buf_len = buf_len;
    event_p->offset_rd = 0;
    event_p->offset_wr = 0;
    event_p->seg_data_len = 0;
    event_p->seg_data_left = 0;
    event_p->event_send = 0;
    event_p->event_ack = 0;
    event_p->err_ovflow = 0;
    sem_init(&event_p->sem);

    ((ring_buf_seg_t *) buf_p)->len = 0;
}


/* ********************************************************* */
/* ********************************************************* */

void *ring_buf_event_seg_get(ring_buf_event_t *event_p, uint32_t len_req)
{
    ring_buf_seg_t *seg_p;
    uint32_t  offset_wr, offset_rd;
    uint32_t  len_avl, len_seg;

    len_seg = len_req + sizeof(ring_buf_seg_t);
    if (len_seg > DRV_BUF_SEG_MAX) {
        return NULL;
    }

    offset_wr = event_p->offset_wr;
    offset_rd = event_p->offset_rd;

    if (offset_wr >= offset_rd) {
        if (offset_wr == offset_rd &&
            event_p->event_ack != event_p->event_send) {
            event_p->err_ovflow++;
            return NULL;
        }

        len_avl = event_p->buf_len - offset_wr;
        if (len_avl < len_seg) {
            ((ring_buf_seg_t *) (event_p->buf_p + offset_wr))->len = DRV_BUF_SEG_LEN_RESET;
            event_p->enevt_wrap++;
            event_p->offset_wr = offset_wr = 0;
            len_avl = offset_rd;
        }
    } else {
        len_avl = offset_rd - offset_wr;
    }

    if (len_avl >= len_seg) {
        seg_p = (ring_buf_seg_t *) (event_p->buf_p + offset_wr);
        event_p->buf_seg_p = (char *) seg_p + sizeof(ring_buf_seg_t);
        event_p->seg_data_len = len_req;
        event_p->seg_data_left = event_p->seg_data_len;
        return (void *) event_p->buf_seg_p;

    } else {
        event_p->err_ovflow++;
        return NULL;
    }
}


/* ********************************************************* */
/* ********************************************************* */

void ring_buf_event_post(ring_buf_event_t *event_p)
{
    ring_buf_seg_t *seg_p;
    uint32_t  len;
    uint32_t  offset;
    
    offset = event_p->offset_wr;
    seg_p = (ring_buf_seg_t *) (event_p->buf_p + offset);
    len = event_p->seg_data_len - event_p->seg_data_left;
    seg_p->len = len;

    offset += sizeof(ring_buf_seg_t) + len;
    if (offset >= event_p->buf_len) {
        offset = 0;
    }
    event_p->offset_wr = offset;
    event_p->seg_data_left = 0;
    
    event_p->byte_send += len;
    event_p->event_send++;
    sem_post(&event_p->sem);
}

void ring_buf_event_wait(ring_buf_event_t *event_p, char **buf_p, uint32_t *buf_len_p)
{
    ring_buf_seg_t *seg_p;
    
    sem_wait(&event_p->sem);
    event_p->event_ack++;

    seg_p = (ring_buf_seg_t *) (event_p->buf_p + event_p->offset_rd);

    if (seg_p->len) {
        if (seg_p->len == DRV_BUF_SEG_LEN_RESET) {
            seg_p = (ring_buf_seg_t *) (event_p->buf_p);
            event_p->offset_rd = 0;
            event_p->enevt_wrap_ack++;
        }
        *buf_p = (char *) seg_p + sizeof(ring_buf_seg_t);
        *buf_len_p = seg_p->len;

    } else {
        *buf_p = NULL;
        *buf_len_p = 0;
    }
}

void ring_buf_event_seg_free(ring_buf_event_t *event_p)
{
    ring_buf_seg_t *seg_p;
    uint32_t  len, offset;
    
    offset = event_p->offset_rd;
    seg_p = (ring_buf_seg_t *) (event_p->buf_p + offset);

    len = seg_p->len;
    event_p->byte_ack += len;

    offset += (sizeof(ring_buf_seg_t) + len);
    if (offset >= event_p->buf_len) {
        offset = 0;
    }
    event_p->offset_rd = offset;
}


/* ********************************************************* */
/* ********************************************************* */

void ring_buf_event_dump(ring_buf_event_t *event_p)
{
    printf("drv_buf_event ......... \n");
    
    printf("  buf_p   : 0x%lx\n", (uint32_t) event_p->buf_p);
    printf("  buf_len : %d (0x%x)\n", event_p->buf_len, event_p->buf_len);
    printf("  ofst_rd : %d\n", event_p->offset_rd);
    printf("  ofst_wr : %d\n", event_p->offset_wr);
    printf("  seg_p   : 0x%lx\n", (uint32_t) event_p->buf_seg_p);
    printf("  seg_len : %d (0x%x)\n", event_p->seg_data_len, event_p->seg_data_len);
    printf("  seg_left: %u\n", event_p->seg_data_left);
    printf("  evnt_snd: %lu\n", event_p->event_send);
    printf("  evnt_ack: %lu\n", event_p->event_ack);
    printf("  byte_snd: %lu\n", event_p->byte_send);
    printf("  byte_ack: %lu\n", event_p->byte_ack);
    printf("  evnt_wrp: %lu (%lu)\n", event_p->enevt_wrap, event_p->enevt_wrap_ack);
    printf("  err_ov  : %lu\n", event_p->err_ovflow);
    printf("\n");
}


void ring_buf_event_buf_dump(ring_buf_event_t *event_p)
{
    uint32_t  n;

    for (n = 0; n < event_p->buf_len; ++n) {
        if ((n % 10) == 0 && n != 0) {
            printf("\n");
        }
        if ((n % 10) == 0) {
            printf(" %4lu - ", n);
        }
        printf("%02x ", (uint8_t) event_p->buf_p[n]);
    }
    printf("\n\n");
}
