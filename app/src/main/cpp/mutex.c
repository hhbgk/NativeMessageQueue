#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include "mutex.h"

Mutex *CreateMutex(void)
{
    Mutex *mutex;
    mutex = (Mutex *) malloc(sizeof(Mutex));
    if (!mutex)
        return NULL;

    if (pthread_mutex_init(&mutex->id, NULL) != 0) {
        free(mutex);
        return NULL;
    }

    return mutex;
}

void DestroyMutex(Mutex *mutex)
{
    if (mutex) {
        pthread_mutex_destroy(&mutex->id);
        free(mutex);
    }
}

void DestroyMutexP(Mutex **mutex)
{
    if (mutex) {
        DestroyMutex(*mutex);
        *mutex = NULL;
    }
}

int LockMutex(Mutex *mutex)
{
    assert(mutex);
    if (!mutex)
        return -1;

    return pthread_mutex_lock(&mutex->id);
}

int UnlockMutex(Mutex *mutex)
{
    assert(mutex);
    if (!mutex)
        return -1;

    return pthread_mutex_unlock(&mutex->id);
}

Cond *CreateCond(void)
{
    Cond *cond;
    cond = (Cond *) malloc(sizeof(Cond));
    if (!cond)
        return NULL;

    if (pthread_cond_init(&cond->id, NULL) != 0) {
        free(cond);
        return NULL;
    }

    return cond;
}

void DestroyCond(Cond *cond)
{
    if (cond) {
        pthread_cond_destroy(&cond->id);
        free(cond);
    }
}

void DestroyCondP(Cond **cond)
{
    if (cond) {
        DestroyCond(*cond);
        *cond = NULL;
    }
}

int CondSignal(Cond *cond)
{
    assert(cond);
    if (!cond)
        return -1;

    return pthread_cond_signal(&cond->id);
}

int CondBroadcast(Cond *cond)
{
    assert(cond);
    if (!cond)
        return -1;

    return pthread_cond_broadcast(&cond->id);
}

int CondWaitTimeout(Cond *cond, Mutex *mutex, uint32_t ms)
{
    int retval;
    struct timeval delta;
    struct timespec abstime;

    assert(cond);
    assert(mutex);
    if (!cond || !mutex) {
        return -1;
    }

    gettimeofday(&delta, NULL);

    abstime.tv_sec = delta.tv_sec + (ms / 1000);
    abstime.tv_nsec = (delta.tv_usec + (ms % 1000) * 1000) * 1000;
    if (abstime.tv_nsec > 1000000000) {
        abstime.tv_sec += 1;
        abstime.tv_nsec -= 1000000000;
    }

    while (1) {
        retval = pthread_cond_timedwait(&cond->id, &mutex->id, &abstime);
        if (retval == 0)
            return 0;
        else if (retval == EINTR)
            continue;
        else if (retval == ETIMEDOUT)
            return MUTEX_TIMEDOUT;
        else
            break;
    }

    return -1;
}

int CondWait(Cond *cond, Mutex *mutex)
{
    assert(cond);
    assert(mutex);
    if (!cond || !mutex)
        return -1;

    return pthread_cond_wait(&cond->id, &mutex->id);
}
