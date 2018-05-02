/*
 * 1. data_types size
 * 2. mem_pool size
 * 3. offset of object in mem_pool
 * 4. mem_pool put/get simultor
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "mem-pool.h"


#define MIN_SLEEP 10000000
#define ITER 100



void size_check(){
    printf("--------------types size check--------------\n");

    printf("sizeof char %lu\n", sizeof(char));
    printf("sizeof short %lu\n", sizeof(short));
    printf("sizeof int %lu\n", sizeof(int));
    printf("sizeof long %lu\n", sizeof(long));
    printf("sizeof long long %lu\n", sizeof(long long));
    printf("sizeof pointer %lu\n", sizeof(void *));
    printf("sizeof size_t %lu\n", sizeof(size_t));
}


struct req_struct {
    int from;
    char* name;
    int to;
    double val;
    char str[1240];
};
typedef struct req_struct req_t;


struct worker_args_struct{
    int id;
    struct mem_pool* pool;
};
typedef struct worker_args_struct worker_args_t;


void* do_work(void* worker_args){
    worker_args_t* args = (worker_args_t*) worker_args;
    srand(time(NULL));
    struct timespec dur;
    dur.tv_sec = 0;
    req_t** obj = (req_t**) malloc(sizeof(req_t *) * ITER);
    while(1){
        for(int t = 0; t < ITER; t++){
            obj[t] = mem_get0(args->pool);
            char* str = obj[t];
            size_t len = sizeof(req_t);
            for(int i = 0; i < len; i++) str[i] = rand()%26+'a';
        }
        for(int t = 0; t < ITER; t++){
            dur.tv_nsec = rand();
            if(dur.tv_nsec < MIN_SLEEP)
                dur.tv_nsec +=  MIN_SLEEP;
            nanosleep(&dur, NULL);
            mem_put(obj[t]);
        }
    }
    free(obj);
}


void monitor_mem_pool(struct mem_pool* mem_pool){
    if(!mem_pool) return;
    LOCK(&mem_pool->lock);
    {
        struct timeval now;
        gettimeofday(&now, NULL);
        struct tm* tm;
        tm = localtime(&now.tv_sec);
        char buf[30];
        strftime(buf, 30, "%Y-%m-%d %H:%M:%S", tm);
        printf("monitor at %s\n", buf);
        printf("mempool list next %p, prev %p\n", mem_pool->list.next, mem_pool->list.prev);
        printf("mempool hot_count %d\n", mem_pool->hot_count);
        printf("mempool cold_count %d\n", mem_pool->cold_count);
        printf("mempool alloc_count %ld\n", mem_pool->alloc_count);
        printf("mempool max_alloc %d\n", mem_pool->max_alloc);
        printf("-----------------\n");
    }
    UNLOCK(&mem_pool->lock);
}

void* start_monitor(void* worker_args){
    worker_args_t* args = (worker_args_t*) worker_args;
    while(1){
        sleep(2);
        monitor_mem_pool(args->pool);
    }
}


void mem_pool_check(){
    printf("-------------mem_pool size/offset-------------\n");

    printf("sizeof req_t %lu\n", sizeof(req_t));
    struct mem_pool * req_pool = mem_pool_new(req_t, 4096);
    printf("sizeof mem_pool %lu\n", sizeof(struct mem_pool));
    printf("address of req_pool %p\n", req_pool);
    printf("address of req_pool list new %p\n", &req_pool->list.next);
    printf("address of req_pool list prev  %p\n", &req_pool->list.prev);
    printf("address of req_pool hot_count %p\n", &req_pool->hot_count);
    printf("address of req_pool lock %p\n", &req_pool->cold_count);
    printf("address of req_pool paddedsize %p\n", &req_pool->padded_sizeof_type);
    printf("address of req_pool pool_begin %p\n", &req_pool->pool);
    printf("address of req_pool pool_end %p\n", &req_pool->pool_end);
    printf("address of req_pool realsize %p\n", &req_pool->real_sizeof_type);
    printf("address of req_pool alloc_count %p\n", &req_pool->alloc_count);
    printf("address of req_pool pool_miss %p\n", &req_pool->pool_misses);
    printf("address of req_pool max_alloc %p\n", &req_pool->max_alloc);
    printf("address of req_pool cur_stdalloc %p\n", &req_pool->curr_stdalloc);
    printf("address of req_pool max_stdalloc %p\n", &req_pool->max_stdalloc);
    printf("address of req_pool name %p\n", &req_pool->name);
    printf("address of req_pool global_list %p\n", &req_pool->global_list);


    printf("-------------mem_pool object size/offset-------------\n");

    printf("address of chunk[0] %p %p\n", req_pool->list.next, req_pool->pool);
    printf("address of chunk head %p\n", req_pool->list.next);
    void* head = req_pool->pool;
    struct req_t* req_ptr = mem_pool_chunkhead2ptr(head);
    printf("address of chunk mem_pool* %p\n", mem_pool_from_ptr(head));
    printf("address of chunk in_use %p\n", head + GF_MEM_POOL_LIST_BOUNDARY + GF_MEM_POOL_PTR);
    printf("address of chunk data ptr %p\n", req_ptr);

    struct mem_pool * req_pool2 = mem_pool_new(req_t, 1);
    printf("address of req_pool2 %p\n", req_pool2);
    printf("address of req_pool2 begin %p end %p\n", req_pool2->pool, req_pool2->pool_end);

    struct mem_pool * req_pool3 = mem_pool_new(req_t, 1);
    printf("address of req_pool3 %p\n", req_pool3);
    return;


    int num_threads = 16;
    worker_args_t* args = malloc(sizeof(worker_args_t) * num_threads);
    pthread_t* workers = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    int i;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    for(i = 0; i < num_threads; i++){
        args[i].id = i;
        args[i].pool = req_pool;
        pthread_create(&workers[i], &attr, do_work, &args[i]);
    }

    worker_args_t monitor_args = {
        .id=-1,
        .pool=req_pool
    };
    pthread_t monitor_worker;
    pthread_create(&monitor_worker, NULL, start_monitor, &monitor_args);

    for(int i = 0; i < num_threads; i++){
        pthread_join(workers[i], NULL);
    }
    pthread_join(monitor_worker, NULL);
}




struct Node{
    unsigned long x;
};

void align_check(){
    printf("-------------unaligned read/write-------------\n");
    printf("sizeof Node %lu\n", sizeof(struct Node));

    void* data = calloc(256, 1);
    printf("address of data %p\n", data);
    memset(data, 1, 4);
    memset(data+4, 0xff, 8);

    printf("value of Node.x unaligned %lx\n", ((struct Node*)(data+1))->x);

}

void clean(void *ptr){

}

int main(){
    //printf("%d\n", RAND_MAX);
    size_check();
    mem_pool_check();
    //align_check();
    return 0;
}
