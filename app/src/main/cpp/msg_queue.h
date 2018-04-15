#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H

#include "mutex.h"
#include <assert.h>
#include <stdlib.h>

#define MSG_FLUSH                       0

typedef struct AVMessage {
    int what;
    int arg1;
    int arg2;
    int len;
    void *obj;
    void (*free_l)(void *obj);
    struct AVMessage *next;
} AVMessage;

typedef struct MessageQueue {
    AVMessage *first_msg, *last_msg;
    int nb_messages;
    int abort_request;
    Mutex *mutex;
    Cond *cond;

    AVMessage *recycle_msg;
    int recycle_count;
    int alloc_count;
} MessageQueue;

inline static void msg_free_res(AVMessage *msg)
{
    if (!msg || !msg->obj)
        return;
    assert(msg->free_l);
    msg->free_l(msg->obj);
    msg->obj = NULL;
}

inline static int msg_queue_put_private(MessageQueue *q, AVMessage *msg)
{
    AVMessage *msg1;

    if (q->abort_request)
        return -1;

    msg1 = q->recycle_msg;
    if (msg1) {
        q->recycle_msg = msg1->next;
        q->recycle_count++;
    } else {
        q->alloc_count++;
        msg1 = (AVMessage *) malloc(sizeof(AVMessage));
    }
    if (!msg1)
        return -1;

    *msg1 = *msg;
    msg1->next = NULL;

    if (!q->last_msg)
        q->first_msg = msg1;
    else
        q->last_msg->next = msg1;
    q->last_msg = msg1;
    q->nb_messages++;
    CondSignal(q->cond);
    return 0;
}

inline static int msg_queue_put(MessageQueue *q, AVMessage *msg)
{
    int ret;

    LockMutex(q->mutex);
    ret = msg_queue_put_private(q, msg);
    UnlockMutex(q->mutex);

    return ret;
}

inline static void msg_init_msg(AVMessage *msg)
{
    memset(msg, 0, sizeof(AVMessage));
}

inline static void msg_queue_put_simple1(MessageQueue *q, int what)
{
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg_queue_put(q, &msg);
}

inline static void msg_queue_put_simple2(MessageQueue *q, int what, int arg1)
{
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg_queue_put(q, &msg);
}

inline static void msg_queue_put_simple3(MessageQueue *q, int what, int arg1, int arg2)
{
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    msg_queue_put(q, &msg);
}

inline static void msg_obj_free_l(void *obj)
{
    free(obj);
}

inline static void msg_queue_put_simple4(MessageQueue *q, int what, int arg1, int arg2, void *obj, int obj_len)
{
    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = what;
    msg.arg1 = arg1;
    msg.arg2 = arg2;
    msg.len = obj_len;
    msg.obj = malloc((size_t) obj_len);
    memcpy(msg.obj, obj, (size_t) obj_len);
    msg.free_l = msg_obj_free_l;
    msg_queue_put(q, &msg);
}

inline static void msg_queue_init(MessageQueue *q)
{
    memset(q, 0, sizeof(MessageQueue));
    q->mutex = CreateMutex();
    q->cond = CreateCond();
    q->abort_request = 1;
}

inline static void msg_queue_flush(MessageQueue *q)
{
    AVMessage *msg, *msg1;

    LockMutex(q->mutex);
    for (msg = q->first_msg; msg != NULL; msg = msg1) {
        msg1 = msg->next;
        msg->next = q->recycle_msg;
        q->recycle_msg = msg;
    }
    q->last_msg = NULL;
    q->first_msg = NULL;
    q->nb_messages = 0;
    UnlockMutex(q->mutex);
}

inline static void msg_queue_destroy(MessageQueue *q)
{
    msg_queue_flush(q);

    LockMutex(q->mutex);
    while(q->recycle_msg) {
        AVMessage *msg = q->recycle_msg;
        if (msg)
            q->recycle_msg = msg->next;
        msg_free_res(msg);
        free(msg);
    }
    UnlockMutex(q->mutex);

    DestroyMutex(q->mutex);
    DestroyCond(q->cond);
}

inline static void msg_queue_abort(MessageQueue *q)
{
    LockMutex(q->mutex);

    q->abort_request = 1;

    CondSignal(q->cond);

    UnlockMutex(q->mutex);
}

inline static void msg_queue_start(MessageQueue *q)
{
    LockMutex(q->mutex);
    q->abort_request = 0;

    AVMessage msg;
    msg_init_msg(&msg);
    msg.what = MSG_FLUSH;
    msg_queue_put_private(q, &msg);
    UnlockMutex(q->mutex);
}

/* return < 0 if aborted, 0 if no msg and > 0 if msg.  */
inline static int msg_queue_get(MessageQueue *q, AVMessage *msg, int block)
{
    AVMessage *msg1;
    int ret;

    LockMutex(q->mutex);

    for (;;) {
        if (q->abort_request) {
            ret = -1;
            break;
        }

        msg1 = q->first_msg;
        if (msg1) {
            q->first_msg = msg1->next;
            if (!q->first_msg)
                q->last_msg = NULL;
            q->nb_messages--;
            *msg = *msg1;
            msg1->obj = NULL;
            msg1->next = q->recycle_msg;
            q->recycle_msg = msg1;
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            CondWait(q->cond, q->mutex);
        }
    }
    UnlockMutex(q->mutex);
    return ret;
}

inline static void msg_queue_remove(MessageQueue *q, int what)
{
    AVMessage **p_msg, *msg, *last_msg;
    LockMutex(q->mutex);

    last_msg = q->first_msg;

    if (!q->abort_request && q->first_msg) {
        p_msg = &q->first_msg;
        while (*p_msg) {
            msg = *p_msg;

            if (msg->what == what) {
                *p_msg = msg->next;
                msg_free_res(msg);
                msg->next = q->recycle_msg;
                q->recycle_msg = msg;
                q->nb_messages--;
            } else {
                last_msg = msg;
                p_msg = &msg->next;
            }
        }

        if (q->first_msg) {
            q->last_msg = last_msg;
        } else {
            q->last_msg = NULL;
        }
    }

    UnlockMutex(q->mutex);
}

#endif
