#ifndef LOW_LEVEL_LIB_MK_RINGBUFFER_H
#define LOW_LEVEL_LIB_MK_RINGBUFFER_H

#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include "../config.h"
#include "../logging/statistics.h"


#define LOCK(ringbuffer) (pthread_spin_lock(&(ringbuffer).lock))
#define UNLOCK(ringbuffer) (pthread_spin_unlock(&(ringbuffer).lock))

#define MK_RINGBUFFER_HEADER(name, itemtype, size)                                     \
    typedef struct {                                                                   \
        uint32_t head;                                                                   \
        uint32_t tail;                                                                   \
        pthread_spinlock_t lock;                                                       \
        volatile bool empty;                                                           \
        volatile bool full;                                                            \
        itemtype data[size];                                                           \
        int dropped;                                                                   \
    } name##_ringbuffer_t;                                                             \
    extern name##_ringbuffer_t name##_ringbuffer;                                      \
    void name##_ringbuffer_init();                                                     \
    bool name##_ringbuffer_empty();                                                    \
    bool name##_ringbuffer_full();                                                     \
    int name##_ringbuffer_length();                                                    \
    int name##_ringbuffer_put(const itemtype *item);                                   \
    void name##_ringbuffer_ensure_put(const itemtype *item);                           \
    int name##_ringbuffer_get(itemtype *item);


#define MK_RINGBUFFER_IMPL(name, itemtype, size, stat)                                       \
    name##_ringbuffer_t name##_ringbuffer;                                             \
    void name##_ringbuffer_init() {                                                    \
        name##_ringbuffer.head = 0;                                                    \
        name##_ringbuffer.tail = 0;                                                    \
        name##_ringbuffer.dropped = 0;                                                 \
        name##_ringbuffer.empty = true;                                                \
        name##_ringbuffer.full = false;                                                \
        pthread_spin_init(&name##_ringbuffer.lock, 0);                                 \
    }                                                                                  \
                                                                                       \
    inline bool name##_ringbuffer_empty() {                                            \
        return name##_ringbuffer.empty;                                                \
    }                                                                                  \
                                                                                       \
    inline bool name##_ringbuffer_full() {                                             \
        return name##_ringbuffer.full;                                                 \
    }                                                                                  \
                                                                                       \
    int name##_ringbuffer_length() {                                                   \
        int res;                                                                       \
        LOCK(name##_ringbuffer);                                                       \
        if (name##_ringbuffer.full) res = size;                                        \
        else res = (name##_ringbuffer.head + (size) - name##_ringbuffer.tail) % (size);  \
        UNLOCK(name##_ringbuffer);                                                     \
        return res;                                                                    \
    }                                                                                  \
                                                                                       \
    int name##_ringbuffer_get(itemtype *item) {                                        \
        if (name##_ringbuffer.empty) {                                                 \
            return -1;                                                                 \
        }                                                                              \
                                                                                       \
        LOCK(name##_ringbuffer);                                                       \
        memcpy(item, &name##_ringbuffer.data[name##_ringbuffer.tail], sizeof(itemtype));  \
        name##_ringbuffer.tail = (name##_ringbuffer.tail + 1) % (size);                \
        name##_ringbuffer.empty = name##_ringbuffer.head == name##_ringbuffer.tail;    \
        name##_ringbuffer.full = false;                                                \
        UNLOCK(name##_ringbuffer);                                                     \
                                                                                       \
        return 0;                                                                      \
    }                                                                                  \
                                                                                       \
    int name##_ringbuffer_put(const itemtype *item) {                                  \
        if (name##_ringbuffer.full) {                                                  \
            INC_STAT(stat)                                                             \
            return 1;                                                                  \
        }                                                                              \
        LOCK(name##_ringbuffer);                                                       \
        if (name##_ringbuffer.full) {                                                  \
            INC_STAT(stat)                                                             \
            UNLOCK(name##_ringbuffer);                                                 \
            return 1;                                                                  \
        }                                                                              \
        memcpy(&name##_ringbuffer.data[name##_ringbuffer.head], item, sizeof(itemtype));  \
        name##_ringbuffer.head = (name##_ringbuffer.head + 1) % (size);                \
        name##_ringbuffer.full = name##_ringbuffer.head == name##_ringbuffer.tail;     \
        name##_ringbuffer.empty = false;                                               \
        UNLOCK(name##_ringbuffer);                                                     \
        return 0;                                                                      \
    }                                                                                  \
	                                                                                   \
	void name##_ringbuffer_ensure_put(const itemtype *item) {                          \
        while (1) {                                                                    \
            while (name##_ringbuffer.full);                                            \
            LOCK(name##_ringbuffer);                                                   \
            if (name##_ringbuffer.full) {                                              \
                UNLOCK(name##_ringbuffer);                                             \
                continue;                                                              \
            }                                                                          \
            memcpy(&name##_ringbuffer.data[name##_ringbuffer.head], item, sizeof(itemtype));  \
            name##_ringbuffer.head = (name##_ringbuffer.head + 1) % (size);            \
            name##_ringbuffer.full = name##_ringbuffer.head == name##_ringbuffer.tail; \
            name##_ringbuffer.empty = false;                                           \
            UNLOCK(name##_ringbuffer);                                                 \
            return;                                                                    \
        }                                                                              \
    }
#endif //LOW_LEVEL_LIB_MK_RINGBUFFER_H
