// Copyright 2021-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <modules/pm.h>
#include "sys_driver.h"
#include "pm_debug.h"
#include <os/mem.h>

/****************************************************************************
 * Sleep Callback Management System
 ****************************************************************************/

/* Callback node structure with priority support
 * 
 * Memory layout (32-bit ARM, aligned):
 *   callback:  4 bytes
 *   arg:       4 bytes
 *   priority:  1 byte
 *   padding:   3 bytes (compiler alignment)
 *   next:      4 bytes
 *   Total:     16 bytes per node
 */
typedef struct sleep_callback_node {
	sleep_callback_t callback;           /* Callback function pointer */
	void *arg;                           /* User argument */
	uint8_t priority;                    /* Execution priority: 0=highest, 255=lowest */
	struct sleep_callback_node *next;    /* Next node in sorted list */
} sleep_callback_node_t;

/* Global callback list heads - maintained in sorted order by priority */
static sleep_callback_node_t *g_pre_sleep_callback_head  = NULL;   /* Called before sleep */
static sleep_callback_node_t *g_post_sleep_callback_head = NULL;  /* Called after wakeup */

bk_err_t bk_pm_pre_sleep_callback_register(sleep_callback_t callback, void *arg, uint8_t priority)
{
	sleep_callback_node_t *new_node, *curr, *prev;
	uint32_t int_level;

	if (!callback) {
		return BK_ERR_PARAM;
	}

	/* Allocate new node */
	new_node = (sleep_callback_node_t *)os_malloc(sizeof(sleep_callback_node_t));
	if (!new_node) {
		return BK_ERR_NO_MEM;
	}

	/* Initialize node */
	new_node->callback = callback;
	new_node->arg = arg;
	new_node->priority = priority;

	/* Insert in sorted order by priority (lower value = higher priority)
	 * Maintains sorted list for efficient execution
	 */
	int_level = rtos_disable_int();
	
	curr = g_pre_sleep_callback_head;
	prev = NULL;
	
	/* Find insertion point: insert before first node with lower priority (higher value) */
	while (curr && curr->priority <= priority) {
		prev = curr;
		curr = curr->next;
	}
	
	/* Insert node at found position */
	new_node->next = curr;
	if (prev) {
		prev->next = new_node;  /* Insert in middle or end */
	} else {
		g_pre_sleep_callback_head = new_node;  /* Insert at head */
	}
	
	rtos_enable_int(int_level);

	return BK_OK;
}

/**
 * @brief Unregister a sleep callback
 *
 * @param callback  Callback function to remove
 * @return 0 on success, negative if not found
 *
 * Design: O(n) search but efficient removal
 */
bk_err_t bk_pm_pre_sleep_callback_unregister(sleep_callback_t callback)
{
	sleep_callback_node_t *curr, *prev;
	uint32_t int_level;
	int found = 0;

	if (!callback) {
		return BK_ERR_PARAM;
	}

	int_level = rtos_disable_int();

	curr = g_pre_sleep_callback_head;
	prev = NULL;

	/* Search for callback in list */
	while (curr) {
		if (curr->callback == callback) {
			/* Remove from list */
			if (prev) {
				prev->next = curr->next;
			} else {
				g_pre_sleep_callback_head = curr->next;
			}
			
			rtos_enable_int(int_level);
			
			/* Free memory */
			os_free(curr);
			found = 1;
			break;
		}
		prev = curr;
		curr = curr->next;
	}

	if (!found) {
		rtos_enable_int(int_level);
	}

	return found ? BK_OK : BK_ERR_NOT_FOUND;
}

bk_err_t bk_pm_pre_sleep_callback_execute(void)
{
	sleep_callback_node_t *curr;
	volatile uint32_t int_level = 0;
	
	/* Get snapshot of list head (minimize critical section) */
	int_level = rtos_disable_int();
	curr = g_pre_sleep_callback_head;
	rtos_enable_int(int_level);

	/* Execute all callbacks with interrupts enabled
	 * This allows callbacks to perform complex operations
	 */
	while (curr) {
		if (curr->callback) {
			curr->callback(curr->arg);
		}
		curr = curr->next;
	}
	return BK_OK;
}

bk_err_t bk_pm_post_sleep_callback_register(sleep_callback_t callback, void *arg, uint8_t priority)
{
	sleep_callback_node_t *new_node, *curr, *prev;
	uint32_t int_level;

	if (!callback) {
		return BK_ERR_PARAM;
	}

	/* Allocate new node */
	new_node = (sleep_callback_node_t *)os_malloc(sizeof(sleep_callback_node_t));
	if (!new_node) {
		return BK_ERR_NO_MEM;
	}

	/* Initialize node */
	new_node->callback = callback;
	new_node->arg = arg;
	new_node->priority = priority;

	/* Insert in sorted order by priority */
	int_level = rtos_disable_int();
	
	curr = g_post_sleep_callback_head;
	prev = NULL;
	
	/* Find insertion point */
	while (curr && curr->priority <= priority) {
		prev = curr;
		curr = curr->next;
	}
	
	/* Insert node at found position */
	new_node->next = curr;
	if (prev) {
		prev->next = new_node;
	} else {
		g_post_sleep_callback_head = new_node;
	}
	
	rtos_enable_int(int_level);

	return BK_OK;
}

bk_err_t bk_pm_post_sleep_callback_unregister(sleep_callback_t callback)
{
	sleep_callback_node_t *curr, *prev;
	uint32_t int_level;
	int found = 0;

	if (!callback) {
		return BK_ERR_PARAM;
	}

	int_level = rtos_disable_int();

	curr = g_post_sleep_callback_head;
	prev = NULL;

	/* Search for callback in list */
	while (curr) {
		if (curr->callback == callback) {
			/* Remove from list */
			if (prev) {
				prev->next = curr->next;
			} else {
				g_post_sleep_callback_head = curr->next;
			}
			
			rtos_enable_int(int_level);
			
			/* Free memory */
			os_free(curr);
			found = 1;
			break;
		}
		prev = curr;
		curr = curr->next;
	}

	if (!found) {
		rtos_enable_int(int_level);
	}

	return found ? BK_OK : BK_ERR_NOT_FOUND;
}

bk_err_t bk_pm_post_sleep_callback_execute(void)
{
	sleep_callback_node_t *curr;
	volatile uint32_t int_level = 0;
	
	/* Get snapshot of list head (minimize critical section) */
	int_level = rtos_disable_int();
	curr = g_post_sleep_callback_head;
	rtos_enable_int(int_level);

	/* Execute all callbacks with interrupts enabled
	 * This allows callbacks to perform complex operations
	 */
	while (curr) {
		if (curr->callback) {
			curr->callback(curr->arg);
		}
		curr = curr->next;
	}
	return BK_OK;
}