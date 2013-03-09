/*
 * Description: lock free queue, for mulit process or thread read and write
 *     History: damonyang@tencent.com, 2013/03/06, create
 */

# include <stdbool.h>
# include <stdlib.h>
# include <string.h>
# include <strings.h>

# include "oi_shm.h"
# include "lf_queue.h"

# pragma pack(1)

typedef struct
{
    volatile int32_t next;
    volatile int32_t use_falg;
} unit_head;

typedef struct
{
    int32_t unit_size;
    int32_t max_unit_num;

    int32_t p_head;
    int32_t p_tail;

    volatile int32_t w_tail; /* 下次写开始位置 */

    volatile int32_t w_len; /* 写计数器 */
    volatile int32_t r_len; /* 读计数器 */
} queue_head;

# pragma pack()

# define LIST_END (-1)

# define UNIT_HEAD(queue, offset) ((unit_head *)((queue) + sizeof(queue_head) + \
            ((offset) + 1) * (((queue_head *)(queue))->unit_size + sizeof(unit_head))))
# define UNIT_DATA(queue, offset) ((void *)UNIT_HEAD(queue, offset) + sizeof(unit_head))

int lf_queue_init(lf_queue *queue, key_t shm_key, int32_t unit_size, int32_t max_unit_num)
{
    if (!queue || !unit_size || !max_unit_num)
        return -2;

    if (unit_size > INT32_MAX - sizeof(unit_head))
        return -3;

    /* 获取内存 */
    int32_t unit_size_real = sizeof(unit_head) + unit_size;
    int32_t max_unit_num_rail = max_unit_num + 2;
    size_t  mem_size = sizeof(queue_head) + unit_size_real * max_unit_num_rail;

    void *memory = NULL;
    bool old_shm = false;

    if (shm_key)
    {
        int ret = GetShm3(&memory, shm_key, mem_size, 0666 | IPC_CREAT);
        if (ret < 0)
            return -1;
        else if (ret == 0)
            old_shm = true;
    }
    else
    {
        if ((memory = malloc(mem_size)) == NULL)
            return -1;

        bzero(memory, mem_size);
    }

    *queue = memory;

    /* 如果是新内存则进行初始化 */
    if (!old_shm)
    {
        volatile queue_head *q_head = *queue;

        q_head->unit_size = unit_size;
        q_head->max_unit_num = max_unit_num;
        q_head->p_head = LIST_END;
        q_head->p_tail = LIST_END;

        UNIT_HEAD(*queue, LIST_END)->next = LIST_END;
    }

    return 0;
}

int lf_queue_push(lf_queue queue, void *unit)
{
    if (!queue || !unit)
        return -2;

    volatile queue_head * head = queue;
    volatile unit_head * u_head;

    /* 检查队列是否可写 */
    int32_t w_len;
    do
    {
        w_len = head->w_len;
        if (head->w_len >= head->max_unit_num)
            return -1;
    } while (!__sync_bool_compare_and_swap(&head->w_len, w_len, w_len + 1));

    /* 为新单元分配内存 */
    int32_t w_tail, old_w_tail;
    do
    {
        do
        {
            old_w_tail = w_tail = head->w_tail;
            w_tail %= (head->max_unit_num + 1);
        } while (!__sync_bool_compare_and_swap(&head->w_tail, old_w_tail, w_tail + 1));

        u_head = UNIT_HEAD(queue, w_tail);
    } while (u_head->use_falg);

    /* 写单元头 */
    unit_head *w_head = UNIT_HEAD(queue, w_tail);
    w_head->next = LIST_END;
    w_head->use_falg = true;

    /* 写数据  */
    memcpy(UNIT_DATA(queue, w_tail), unit, head->unit_size);

    /* 将写完的单元插入链表尾  */
    int32_t p_tail, old_p_tail;
    int reTryTime = 0;
    do
    {
        old_p_tail = p_tail = head->p_tail;
        u_head = UNIT_HEAD(queue, p_tail);

        if ((++reTryTime) >= 3)
        {
            while (u_head->next != LIST_END)
            {
                p_tail = u_head->next;
                u_head = UNIT_HEAD(queue, p_tail);
            }
        }
    } while (!__sync_bool_compare_and_swap(&u_head->next, LIST_END, w_tail));

    /* 更新链表尾 */
    __sync_val_compare_and_swap(&head->p_tail, old_p_tail, w_tail);

    /* 更新读计数器 */
    __sync_fetch_and_add(&head->r_len, 1);

    return 0;
}

int lf_queue_pop(lf_queue queue, void *unit)
{
    if (!queue || !unit)
        return -2;

    volatile queue_head *head = queue;
    volatile unit_head *u_head;

    /* 检查队列是否可读 */
    int32_t r_len;
    do
    {
        r_len = head->r_len;

        if (r_len <= 0)
            return -1;
    } while (!__sync_bool_compare_and_swap(&head->r_len, r_len, r_len - 1));

    /* 从链表头取出一个单元 */
    int32_t p_head;
    do
    {
        p_head = head->p_head;
        u_head = UNIT_HEAD(queue, p_head);
    } while (!__sync_bool_compare_and_swap(&head->p_head, p_head, u_head->next));

    /* 读数据 */
    memcpy(unit, UNIT_DATA(queue, u_head->next), head->unit_size);

    /* 更新单元头 */
    UNIT_HEAD(queue, u_head->next)->use_falg = false;

    /* 更新写计数器 */
    __sync_fetch_and_sub(&head->w_len, 1);

    return 0;
}

int lf_queue_len(lf_queue queue)
{
    if (!queue)
        return -2;

    return ((queue_head *)queue)->r_len;
}

