/*
 * Description: 
 *     History: damonyang@tencent.com, 2013/00/00, create
 */


# include <stdio.h>
# include <unistd.h>
# include <time.h>
# include <signal.h>

# include "lf_queue.h"

int shut_down;

void set_shut_down(int sigo)
{
    shut_down = 1;
}

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

    fprintf(stderr, "queue len: %d\n", lf_queue_len(queue));

    signal(SIGINT, set_shut_down);


    fork();
    fork();

    char file[100];
    sprintf(file, "%d.txt", getpid());

    FILE *fp = fopen(file, "w+");
    
    int n = 0;
    while (1)
    {
        if (shut_down)
            break;

        char buf[100];
        ret = lf_queue_pop(queue, buf);
        if (ret < 0)
        {
            continue;
        }

        fputs(buf, fp);

        ++n;
    }

    fprintf(stderr, "pop: %d\n", n);

    return 0;
}

