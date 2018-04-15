#ifndef MUTEX_H
#define MUTEX_H

#include <stdint.h>
#include <pthread.h>

#define MUTEX_TIMEDOUT  1
#define MUTEX_MAXWAIT   (~(uint32_t)0)

typedef struct _mutex {
    pthread_mutex_t id;
} Mutex;

Mutex  *CreateMutex(void);
void        DestroyMutex(Mutex *mutex);
void        DestroyMutexP(Mutex **mutex);
int         LockMutex(Mutex *mutex);
int         UnlockMutex(Mutex *mutex);

typedef struct _cond {
    pthread_cond_t id;
} Cond;

Cond   *CreateCond(void);
void    DestroyCond(Cond *cond);
void    DestroyCondP(Cond **mutex);
int     CondSignal(Cond *cond);
int     CondBroadcast(Cond *cond);
int     CondWaitTimeout(Cond *cond, Mutex *mutex, uint32_t ms);
int     CondWait(Cond *cond, Mutex *mutex);

#endif

