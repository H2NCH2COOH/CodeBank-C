#ifndef _SYNC_H_
#define _SYNC_H_

#include <pthread.h>
#include <string.h>
#include <errno.h>

#ifndef fatal
    #include <stdlib.h>
    #define fatal() abort()
#endif

#define synced pthread_mutex_t _sync_mutex

#define sync_init(obj) \
    do { \
        pthread_mutexattr_t _sync_mutexattr; \
        pthread_mutexattr_init(&_sync_mutexattr); \
        pthread_mutexattr_settype(&_sync_mutexattr, PTHREAD_MUTEX_RECURSIVE); \
        pthread_mutex_init(&(obj)->_sync_mutex, &_sync_mutexattr); \
    } while (0)

#define sync_acquire(obj) \
    do { \
        int _sync_ret = pthread_mutex_lock(&(obj)->_sync_mutex); \
        if (_sync_ret != 0) { \
            fatal(); \
        } \
    } while(0)

#define sync_try_acquire(obj) \
    ({ \
        int _sync_ret = pthread_mutex_lock(&(obj)->_sync_mutex); \
        if (_sync_ret != 0 && _sync_ret != EBUSY) { \
            fatal(); \
        } \
        _sync_ret == 0; \
    })

#define sync_release(obj) pthread_mutex_unlock(&(obj)->_sync_mutex)

#define _sync_auto_acquire(obj) \
    ({ \
        int _sync_ret = pthread_mutex_lock(&(obj)->_sync_mutex); \
        if (_sync_ret != 0) { \
            fatal(); \
        } \
        &(obj)->_sync_mutex; \
    })
static inline void _sync_auto_release(pthread_mutex_t** mp) {pthread_mutex_unlock(*mp);}

#define sync(obj) pthread_mutex_t* _sync_mutex ## __LINE__ __attribute__ ((__cleanup__(_sync_auto_release))) = _sync_auto_acquire(obj)

#endif /* _SYNC_H_ */
