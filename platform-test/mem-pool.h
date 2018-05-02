/*
  Copyright (c) 2008-2012 Red Hat, Inc. <http://www.redhat.com>
  This file is part of GlusterFS.

  This file is licensed to you under your choice of the GNU Lesser
  General Public License, version 3 or any later version (LGPLv3 or
  later), or the GNU General Public License, version 2 (GPLv2), in all
  cases as published by the Free Software Foundation.
*/

#ifndef _MEM_POOL_H_
#define _MEM_POOL_H_

#include "list.h"
#include "locking.h"
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>



#define GF_MEM_TRAILER_SIZE 8
#define GF_MEM_HEADER_MAGIC  0xCAFEBABE
#define GF_MEM_TRAILER_MAGIC 0xBAADF00D
#define GF_MEM_INVALID_MAGIC 0xDEADC0DE

struct mem_acct_rec {
	const char     *typestr;
        size_t          size;
        size_t          max_size;
        uint32_t        num_allocs;
        uint32_t        total_allocs;
        uint32_t        max_num_allocs;
        gf_lock_t       lock;
};

struct mem_acct {
        uint32_t            num_types;
        /*
         * The lock is only used on ancient platforms (e.g. RHEL5) to keep
         * refcnt increment/decrement atomic.  We could even make its existence
         * conditional on the right set of version/feature checks, but it's so
         * lightweight that it's not worth the obfuscation.
         */
        gf_lock_t           lock;
        unsigned int        refcnt;
        struct mem_acct_rec rec[0];
};

struct mem_header {
        uint32_t        type;
        size_t          size;
        struct mem_acct *mem_acct;
        uint32_t        magic;
        int             padding[8];
};

#define GF_MEM_HEADER_SIZE  (sizeof (struct mem_header))

#ifdef DEBUG
struct mem_invalid {
        uint32_t  magic;
        void     *mem_acct;
        uint32_t  type;
        size_t    size;
        void     *baseaddr;
};
#endif

void *
__gf_calloc (size_t cnt, size_t size, uint32_t type, const char *typestr);

void *
__gf_malloc (size_t size, uint32_t type, const char *typestr);

void *
__gf_realloc (void *ptr, size_t size);

int
gf_vasprintf (char **string_ptr, const char *format, va_list arg);

int
gf_asprintf (char **string_ptr, const char *format, ...);

void
__gf_free (void *ptr);



#define MALLOC(size)       __gf_default_malloc(size)
#define CALLOC(cnt,size)   __gf_default_calloc(cnt,size)
#define REALLOC(ptr,size)  __gf_default_realloc(ptr,size)

#define FREE(ptr)                                       \
        do {                                            \
                if (ptr != NULL) {                      \
                        free ((void *)ptr);             \
                        ptr = (void *)0xeeeeeeee;       \
                }                                       \
        } while (0)

#define GF_CALLOC(nmemb, size, type) __gf_calloc (nmemb, size, type, #type)

#define GF_MALLOC(size, type)  __gf_malloc (size, type, #type)

#define GF_REALLOC(ptr, size)  __gf_realloc (ptr, size)

#define GF_FREE(free_ptr) __gf_free (free_ptr)


struct mem_pool {
        struct list_head  list;
        int               hot_count;
        int               cold_count;
        gf_lock_t         lock;
        unsigned long     padded_sizeof_type;
        void             *pool;
        void             *pool_end;
        int               real_sizeof_type;
        uint64_t          alloc_count;
        uint64_t          pool_misses;
        int               max_alloc;
        int               curr_stdalloc;
        int               max_stdalloc;
        char             *name;
        struct list_head  global_list;
};

#define GF_MEM_POOL_LIST_BOUNDARY        (sizeof(struct list_head))
#define GF_MEM_POOL_PTR                  (sizeof(struct mem_pool*))
#define GF_MEM_POOL_PAD_BOUNDARY         (GF_MEM_POOL_LIST_BOUNDARY  + GF_MEM_POOL_PTR + sizeof(int))
#define mem_pool_chunkhead2ptr(head)     ((head) + GF_MEM_POOL_PAD_BOUNDARY)
#define mem_pool_ptr2chunkhead(ptr)      ((ptr) - GF_MEM_POOL_PAD_BOUNDARY)
#define is_mem_chunk_in_use(ptr)         (*ptr == 1)
#define mem_pool_from_ptr(ptr)           ((ptr) + GF_MEM_POOL_LIST_BOUNDARY)



struct mem_pool *
mem_pool_new_fn (unsigned long sizeof_type, unsigned long count, char *name);

#define mem_pool_new(type,count) mem_pool_new_fn (sizeof(type), count, #type)

void mem_put (void *ptr);
void *mem_get (struct mem_pool *pool);
void *mem_get0 (struct mem_pool *pool);

void mem_pool_destroy (struct mem_pool *pool);

#endif /* _MEM_POOL_H */
