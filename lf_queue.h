/*
 * Description: lock free queue, for mulit process or thread read and write
 *     History: damonyang@tencent.com, 2013/03/06, create
 */

# pragma once

# ifdef __cplusplus
extern "C" {
# endif

# include <stdint.h>
# include <sys/types.h>

typedef void * lf_queue;

/* 
 * pragma:
 *      shm_key: 不为 0 使用共享内存，否则使用 malloc 获取内存
 *      unit_size: 数据单元长度
 *      max_unit_num: 队列最大长度
 * return:
 *      <  0: error
 *      == 0: success
 */
int lf_queue_init(lf_queue *queue, key_t shm_key, int32_t unit_size, int32_t max_unit_num);

/*
 * return:
 *      <  -1: error
 *      == -1: full
 *      ==  0: success
 */
int lf_queue_push(lf_queue queue, void *unit);

/*
 * return:
 *      <  -1: error
 *      == -1: empty
 *      ==  0: success
 */
int lf_queue_pop(lf_queue queue, void *unit);

/*
 * return:
 *      <  0: error
 *      >= 0: len
 */
int lf_queue_len(lf_queue queue);

# ifdef __cplusplus
}
# endif

