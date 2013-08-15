/*
 * Description: 
 *     History: damonyang@tencent.com, 2013/00/00, create
 */


# include <stdio.h>
# include <unistd.h>
# include <time.h>

# include "lf_queue.h"

int main()
{
    int ret;
    lf_queue queue;

    ret = lf_queue_init(&queue, 100, 100, 10000);
    if (ret < 0)
    {
        printf("lf_queue_init error: %d\n", ret);

        return 0;
    }

    fork();
    fork();

    int i;
    int n = 0;
    for (i = 0; i < 100000; ++i)
    {
        char buf[100];

        sprintf(buf, "%d\n", i);

        ret = lf_queue_push(queue, buf);
        if (ret < 0)
        {
            fprintf(stderr, "lf_queue_push error: %d\n", ret);

            break;
        }

        ++n;
    }

    fprintf(stderr, "push: %d\n", n);

    return 0;
}

