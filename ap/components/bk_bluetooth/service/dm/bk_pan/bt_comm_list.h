/**
 * @file bt_comm_list.h
 *
 * @brief Lightweight singly linked list helpers used by Bluetooth services.
 */

#ifndef BT_COMM_LIST_H
#define BT_COMM_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** Linked list node. */
struct bt_comm_list_node_t {
    struct bt_comm_list_node_t *next; /**< Next node. */
    void *data;                       /**< User data pointer. */
};

/** Linked list node type. */
typedef struct bt_comm_list_node_t bt_comm_list_node_t;

/** Linked list container. */
typedef struct bt_comm_list_t {
    bt_comm_list_node_t *head; /**< First node. */
    bt_comm_list_node_t *tail; /**< Last node. */
    uint16_t length;           /**< Number of nodes in the list. */
} bt_comm_list_t;

/**
 * @brief Allocate a new linked list.
 *
 * @return Pointer to the new list on success, otherwise NULL.
 */
bt_comm_list_t *bt_comm_list_new(void);

/**
 * @brief Check whether a linked list is empty.
 *
 * @param list List pointer.
 *
 * @return true if the list is empty or NULL, otherwise false.
 */
bool bt_comm_list_is_empty(const bt_comm_list_t *list);

/**
 * @brief Get the first data pointer in a linked list.
 *
 * @param list List pointer.
 *
 * @return First data pointer on success, otherwise NULL.
 */
void *bt_comm_list_front(const bt_comm_list_t *list);

/**
 * @brief Remove a data pointer from a linked list.
 *
 * @param list List pointer.
 * @param data Data pointer to remove.
 *
 * @return true if the item was removed, otherwise false.
 */
bool bt_comm_list_remove(bt_comm_list_t *list, void *data);

/**
 * @brief Clear all nodes from a linked list.
 *
 * @param list List pointer.
 */
void bt_comm_list_clear(bt_comm_list_t *list);

/**
 * @brief Clear and free a linked list.
 *
 * @param list List pointer.
 */
void bt_comm_list_free(bt_comm_list_t *list);

/**
 * @brief Append a data pointer to a linked list.
 *
 * @param list List pointer.
 * @param data Data pointer to append.
 *
 * @return true on success, otherwise false.
 */
bool bt_comm_list_append(bt_comm_list_t *list, void *data);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*BT_COMM_LIST_H*/
