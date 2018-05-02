/*
  Copyright (c) 2008-2012 Red Hat, Inc. <http://www.redhat.com>
  This file is part of GlusterFS.

  This file is licensed to you under your choice of the GNU Lesser
  General Public License, version 3 or any later version (LGPLv3 or
  later), or the GNU General Public License, version 2 (GPLv2), in all
  cases as published by the Free Software Foundation.
*/

#include "mem-pool.h"
#include "common.h"
#include <stdlib.h>
#include <stdarg.h>


struct mem_pool *
mem_pool_new_fn (unsigned long sizeof_type,
                 unsigned long count, char *name)
{
        struct mem_pool  *mem_pool = NULL;
        unsigned long     padded_sizeof_type = 0;
        void             *pool = NULL;
        int               i = 0;
        int               ret = 0;
        struct list_head *list = NULL;

        if (!sizeof_type || !count) {
                ERROR("mem-pool invalid argu");
                return NULL;
        }
        padded_sizeof_type = sizeof_type + GF_MEM_POOL_PAD_BOUNDARY;

        mem_pool = calloc(sizeof (*mem_pool), 1);
        if (!mem_pool)
                return NULL;

        mem_pool->name = "mem_pool-test";


        LOCK_INIT (&mem_pool->lock);
        INIT_LIST_HEAD (&mem_pool->list);
        INIT_LIST_HEAD (&mem_pool->global_list);

        mem_pool->padded_sizeof_type = padded_sizeof_type;
        mem_pool->real_sizeof_type = sizeof_type;

#ifndef DEBUG
        mem_pool->cold_count = count;
        pool = calloc(count, padded_sizeof_type);
        if (!pool) {
                free(mem_pool);
                return NULL;
        }

        for (i = 0; i < count; i++) {
                list = pool + (i * (padded_sizeof_type));
                INIT_LIST_HEAD (list);
                list_add_tail (list, &mem_pool->list);
        }

        mem_pool->pool = pool;
        mem_pool->pool_end = pool + (count * (padded_sizeof_type));
#endif


out:
        return mem_pool;
}

void*
mem_get0 (struct mem_pool *mem_pool)
{
        void             *ptr = NULL;

        if (!mem_pool) {
                ERROR("mem-pool invalid argu");
                return NULL;
        }

        ptr = mem_get(mem_pool);

        if (ptr)
                memset(ptr, 0, mem_pool->real_sizeof_type);

        return ptr;
}

void *
mem_get (struct mem_pool *mem_pool)
{
        struct list_head *list = NULL;
        void             *ptr = NULL;
        int             *in_use = NULL;
        struct mem_pool **pool_ptr = NULL;

        if (!mem_pool) {
                return NULL;
        }

        LOCK (&mem_pool->lock);
        {
                mem_pool->alloc_count++;
                if (mem_pool->cold_count) { list = mem_pool->list.next;
                        list_del (list);

                        mem_pool->hot_count++;
                        mem_pool->cold_count--;

                        if (mem_pool->max_alloc < mem_pool->hot_count)
                                mem_pool->max_alloc = mem_pool->hot_count;

                        ptr = list;
                        in_use = (ptr + GF_MEM_POOL_LIST_BOUNDARY +
                                  GF_MEM_POOL_PTR);
                        *in_use = 1;

                        goto fwd_addr_out;
                }
                //assert(1 == 2);
                /* This is a problem area. If we've run out of
                 * chunks in our slab above, we need to allocate
                 * enough memory to service this request.
                 * The problem is, these individual chunks will fail
                 * the first address range check in __is_member. Now, since
                 * we're not allocating a full second slab, we wont have
                 * enough info perform the range check in __is_member.
                 *
                 * I am working around this by performing a regular allocation
                 * , just the way the caller would've done when not using the
                 * mem-pool. That also means, we're not padding the size with
                 * the list_head structure because, this will not be added to
                 * the list of chunks that belong to the mem-pool allocated
                 * initially.
                 *
                 * This is the best we can do without adding functionality for
                 * managing multiple slabs. That does not interest us at present
                 * because it is too much work knowing that a better slab
                 * allocator is coming RSN.
                 */
                mem_pool->pool_misses++;
                mem_pool->curr_stdalloc++;
                if (mem_pool->max_stdalloc < mem_pool->curr_stdalloc)
                        mem_pool->max_stdalloc = mem_pool->curr_stdalloc;
                ptr = calloc(1, mem_pool->padded_sizeof_type);

                /* Memory coming from the heap need not be transformed from a
                 * chunkhead to a usable pointer since it is not coming from
                 * the pool.
                 */
        }
fwd_addr_out:
        pool_ptr = mem_pool_from_ptr (ptr);
        *pool_ptr = (struct mem_pool *)mem_pool;
        ptr = mem_pool_chunkhead2ptr (ptr);
        UNLOCK (&mem_pool->lock);

        return ptr;
}


static int
__is_member (struct mem_pool *pool, void *ptr)
{
        if (!pool || !ptr) {
                ERROR("mem-pool invalid argu");
                return -1;
        }

        if (ptr < pool->pool || ptr >= pool->pool_end)
                return 0;

        if ((mem_pool_ptr2chunkhead (ptr) - pool->pool)
            % pool->padded_sizeof_type)
                return -1;

        return 1;
}


void
mem_put (void *ptr)
{
        struct list_head *list = NULL;
        int    *in_use = NULL;
        void   *head = NULL;
        struct mem_pool **tmp = NULL;
        struct mem_pool *pool = NULL;

        if (!ptr) {
                ERROR("mem-pool invalid argu");
                return;
        }

        list = head = mem_pool_ptr2chunkhead (ptr);
        tmp = mem_pool_from_ptr (head);
        if (!tmp) {
                ERROR("mem-pool ptr header is corrupted");
                return;
        }

        pool = *tmp;
        if (!pool) {
                ERROR("mem-pool mem-pool is NULL");
                return;
        }
        LOCK (&pool->lock);
        {

                switch (__is_member (pool, ptr))
                {
                case 1:
                        in_use = (head + GF_MEM_POOL_LIST_BOUNDARY +
                                  GF_MEM_POOL_PTR);
                        if (!is_mem_chunk_in_use(in_use)) {
                                ERROR("mem-pool. mem_put called on freed ptr");
                                break;
                        }
                        pool->hot_count--;
                        pool->cold_count++;
                        *in_use = 0;
                        list_add (list, &pool->list);
                        break;
                case -1:
                        /* For some reason, the address given is within
                         * the address range of the mem-pool but does not align
                         * with the expected start of a chunk that includes
                         * the list headers also. Sounds like a problem in
                         * layers of clouds up above us. ;)
                         */
                        abort ();
                        break;
                case 0:
                        /* The address is outside the range of the mem-pool. We
                         * assume here that this address was allocated at a
                         * point when the mem-pool was out of chunks in mem_get
                         * or the programmer has made a mistake by calling the
                         * wrong de-allocation interface. We do
                         * not have enough info to distinguish between the two
                         * situations.
                         */
                        pool->curr_stdalloc--;
                        free(list);
                        break;
                default:
                        /* log error */
                        break;
                }
        }
        UNLOCK (&pool->lock);
}

