#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <stdatomic.h>
#include "buffer.h"

// tail == head : 缓冲区为空
// (tail + 1) % sz == head : 缓冲区为满
struct ringbuffer_s {
    uint32_t size;  //实际存储数据区域的大小
    uint32_t tail;  //写入指针，理论偏移量
    uint32_t head;  //读取指针，理论偏移量
    uint8_t * buf;  //实际存储数据区域
};

#define min(lth, rth) ((lth)<(rth)?(lth):(rth))

// 检查输入的 num 是否为 2 的幂
static inline int is_power_of_two(uint32_t num) {
    if (num < 2) return 0;
    return (num & (num - 1)) == 0;
}

// 将输入的 num 向上取整为最近的 2 的幂
static inline uint32_t roundup_power_of_two(uint32_t num) {
    if (num == 0) return 2;
    int i = 0;
    for (; num != 0; i++)
        num >>= 1;
    return 1U << i;
}

// 根据大小 sz 创建并初始化一个新的环形缓冲区 buf
buffer_t * buffer_new(uint32_t sz) {
    if (!is_power_of_two(sz)) sz = roundup_power_of_two(sz);
    buffer_t * buf = (buffer_t *)malloc(sizeof(buffer_t) + sz);
    if (!buf) {
        return NULL;
    }
    buf->size = sz;
    buf->head = buf->tail = 0;
    // buf + 1 = buf + sizeof(buffer_t) 字节
    // buf 是一个指向 buffer_t 类型的指针
    buf->buf = (uint8_t *)(buf + 1);
    return buf;
}

// 释放指定的环形缓冲区 r
void buffer_free(buffer_t *r) {
    free(r);
    r = NULL;
}

// 检查环形缓冲区 r 是否为空
static uint32_t
rb_isempty(buffer_t *r) {
    return r->head == r->tail;
}

// 检查环形缓冲区 r 是否为满
static uint32_t
rb_isfull(buffer_t *r) {
    return r->size == (r->tail - r->head);
}

// 返回环形缓冲区 r 当前数据的长度
static uint32_t
rb_len(buffer_t *r) {
    return r->tail - r->head;
}

// 返回环形缓冲区 r 剩余空间的大小
static uint32_t
rb_remain(buffer_t *r) {
    return r->size - r->tail + r->head;
}

// 将长度为 sz 的数据 data 添加到环形缓冲区 r 中
int buffer_add(buffer_t *r, const void *data, uint32_t sz) {
    if (sz > rb_remain(r)) {
        return -1;
    }

    uint32_t i;
    // i = min(欲写入的长度 sz , (当前写指针位置开始，直到缓冲区末尾的可用空间的大小))
    i = min(sz, r->size - (r->tail & (r->size - 1)));

    memcpy(r->buf + (r->tail & (r->size - 1)), data, i);
    memcpy(r->buf, data+i, sz-i);

    r->tail += sz;
    return 0;
}

// 从环形缓冲区 r 中读取长度为 sz 的数据到 data 区域中
int buffer_remove(buffer_t *r, void *data, uint32_t sz) {
    assert(!rb_isempty(r));
    uint32_t i;
    sz = min(sz, r->tail - r->head);

    i = min(sz, r->size - (r->head & (r->size - 1)));
    memcpy(data, r->buf+(r->head & (r->size - 1)), i);
    memcpy(data+i, r->buf, sz-i);

    r->head += sz;
    return sz;
}

// 似乎没有使用到？
int buffer_drain(buffer_t *r, uint32_t sz) {
    if (sz > rb_len(r))
        sz = rb_len(r);
    r->head += sz;
    return sz;
}

// 根据输入的分隔字符串 sep 及其长度 seplen，查找其在形缓冲区 r 是否存在？
// 如果存在，返回分隔字符串 sep 末尾距离 head 的差值
// 如果不存在，返回 0
int buffer_search(buffer_t *r, const char* sep, const int seplen) {
    int i;
    // i 为当前位置距 head 的差值
    for (i = 0; i <= rb_len(r)-seplen; i++) {
        // pos 为当前位置的实际偏移量
        int pos = (r->head + i) & (r->size - 1);
        // 分隔字符串 sep 跨越缓冲区 r 的末尾
        if (pos + seplen > r->size) {
            if (memcmp(r->buf+pos, sep, r->size-pos))
                return 0;
            if (memcmp(r->buf, sep+r->size-pos, pos+seplen-r->size) == 0) {
                return i+seplen;
            }
        }
        // 分隔字符串 sep 没有跨越缓冲区 r 的末尾
        if (memcmp(r->buf+pos, sep, seplen) == 0) {
            return i+seplen;
        }
    }
    return 0;
}

// 返回环形缓冲区 r 当前数据的长度
uint32_t buffer_len(buffer_t *r) {
    return rb_len(r);
}

// 当 rpos > wpos 时，写指针已经回绕，此时不方便 int n = write(fd, buf, sz)执行
// 调整 buf 为重头开始
// 检查环形缓冲区的数据是否跨越了末尾，返回数据的开头地址
uint8_t * buffer_write_atmost(buffer_t *r) {
    uint32_t rpos = r->head & (r->size - 1);
    uint32_t wpos = r->tail & (r->size - 1);
    if (wpos < rpos) {
        uint8_t* temp = (uint8_t *)malloc(r->size * sizeof(uint8_t));
        memcpy(temp, r->buf+rpos, r->size - rpos);
        memcpy(temp+r->size-rpos, r->buf, wpos);
        free(r->buf);
        r->buf = temp;
        return r->buf;
    }
    return r->buf + rpos;
}
