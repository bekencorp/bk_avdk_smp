#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "bk_list.h"

void *idk_list_node_pop(LIST_HEADER_T *head, uint32_t member_offset);

#define ilist_t LIST_HEADER_T

#define ilist_get_node(head, type, member)                  idk_list_node_pop(head, offsetof(type,member))
#define ilist_init(name)                                    LIST_HEAD_INIT(name)
#define ilist_add_head(node, head)                          list_add_head(node, head)
#define ilist_add_tail(node, head)                          list_add_tail(node, head)
#define ilist_del_node(entry)                               list_del(entry)
#define ilist_list_for_each_entry(pos, n, head, member)     list_for_each_entry_safe(pos, n, head, member)
#define ilist_size(list)                                    list_size(list)

typedef void *(*alloc_fn)(size_t size);
typedef void (*free_fn)(void *ptr);

typedef struct
{
    alloc_fn alloc;
    free_fn free;
} allocator_t;

extern const allocator_t allocator_malloc;
extern const allocator_t allocator_calloc;


struct alist_node_t;
typedef struct alist_node_t alist_node_t;

struct alist_t;
typedef struct alist_t alist_t;

typedef void (*alist_free_cb)(void *data);
typedef bool (*alist_iter_cb)(void *data, void *context);

struct alist_node_t
{
    struct alist_node_t *next;
    void *data;
};

typedef struct alist_t
{
    alist_node_t *head;
    alist_node_t *tail;
    size_t length;
    alist_free_cb free_cb;
    const allocator_t *allocator;
} alist_t;


alist_t *alist_new(alist_free_cb callback);
void alist_free(alist_t *alist);
bool alist_is_empty(const alist_t *alist);
bool alist_contains(const alist_t *alist, const void *data);
size_t alist_length(const alist_t *alist);
void *alist_front(const alist_t *alist);
void *alist_back(const alist_t *alist);
alist_node_t *alist_back_node(const alist_t *alist);
bool alist_insert_after(alist_t *alist, alist_node_t *prev_node, void *data);
bool alist_prepend(alist_t *alist, void *data);
bool alist_append(alist_t *alist, void *data);
bool alist_remove(alist_t *alist, void *data);
void alist_clear(alist_t *alist);
alist_node_t *alist_foreach(const alist_t *alist, alist_iter_cb callback,
                            void *context);
alist_node_t *alist_begin(const alist_t *alist);
alist_node_t *alist_end(const alist_t *alist);
alist_node_t *alist_next(const alist_node_t *node);
void *alist_node(const alist_node_t *node);
void *alist_foreach_pop(alist_t *alist, alist_iter_cb callback,
                        void *context);
