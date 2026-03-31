// Copyright 2020-2025 Beken
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

#pragma once

typedef enum {
        INT_CONTROLLER_ID_PRIMARY = 0,
        INT_CONTROLLER_ID_SECONDARY = 1,
        INT_CONTROLLER_ID_TERTIARY = 2,
        INT_CONTROLLER_ID_MAX = 3,
} int_controller_id_t;

#define VECTOR_SYS_ENTRY_CNT (16)

typedef void (*isr_func_t)(void);

/* extend isr_item_t structure */
typedef struct {
        isr_func_t func; /* function pointer */
} isr_item_t;

typedef struct {
        uint32_t id;
        uint32_t isr_capacity;
        isr_item_t *int_list_entry;
        void *private;
} int_controller_t;

/**
 * @brief connect interrupt service routine
 * @param int_ctrl interrupt controller pointer
 * @param int_num interrupt number
 * @param func interrupt service routine function pointer
 * @return BK_OK on success, BK_FAIL on failure
 */
int int_controller_connect(int_controller_t *int_ctrl, uint32_t int_num, isr_func_t func);

/**
 * @brief disconnect interrupt service routine
 * @param int_ctrl interrupt controller pointer
 * @param int_num interrupt number
 * @return BK_OK on success, BK_FAIL on failure
 */
int int_controller_disconnect(int_controller_t *int_ctrl, uint32_t int_num);

/**
 * @brief get interrupt controller capacity
 * @param intc_id interrupt controller ID
 * @return interrupt service routine capacity on success, 0 on failure
 */
unsigned int int_controller_get_capacity(int intc_id);

/**
 * @brief get interrupt controller id
 * @param int_ctrl interrupt controller pointer
 * @return interrupt controller id on success, BK_FAIL on failure
 */
int int_controller_get_intc_id(int_controller_t *int_ctrl);

/**
 * @brief get interrupt controller by id
 * @param intc_id interrupt controller ID
 * @return pointer to interrupt controller, NULL if not found
 */
int_controller_t *int_controller_get_by_id(int intc_id);

/**
 * @brief destroy interrupt controller
 * @param int_ctrl_ptr pointer to interrupt controller pointer
 * @return BK_OK on success, BK_FAIL on failure
 * @note This function will free all resources allocated by int_controller_create
 */
int int_controller_destroy(int_controller_t **int_ctrl_ptr);

/**
 * @brief get interrupt service routine by controller and ID
 * @param intc_id interrupt controller ID (NVIC_0 or NVIC_1)
 * @param isr_id interrupt service routine ID
 * @return pointer to isr_item_t, NULL if not found
 */
isr_item_t* int_controller_get_isr_item_by_id(int intc_id, int isr_id);

/**
 * @brief create interrupt controller
 * @param id interrupt controller id
 * @param isr_capacity interrupt service routine capacity
 * @param vtor_addr vector table offset address (only for primary controller)
 * @return pointer to interrupt controller on success, NULL on failure
 * @note For primary controller, vtor_addr must be provided; for other controllers, vtor_addr must be NULL
 */
int_controller_t *int_controller_create(uint32_t id, uint32_t isr_capacity, void *vtor_addr);

/**
 * @brief connect interrupt service routine by controller ID and interrupt number
 * @param intc_id interrupt controller ID
 * @param int_num interrupt number
 * @param func interrupt service routine function pointer
 * @return BK_OK on success, BK_FAIL on failure
 */
int int_controller_connect_by_intc_id(uint32_t intc_id, uint32_t int_num, isr_func_t func);

/**
 * @brief disconnect interrupt service routine by controller ID and interrupt number
 * @param intc_id interrupt controller ID
 * @param int_num interrupt number
 * @return BK_OK on success, BK_FAIL on failure
 */
int int_controller_disconnect_by_intc_id(uint32_t intc_id, uint32_t int_num);

/**
 * @brief execute interrupt service routine by controller and ID
 * @param intc_id interrupt controller ID (NVIC_0 or NVIC_1)
 * @param isr_id interrupt service routine ID
 * @return execution result, BK_OK on success, BK_FAIL on failure
 */
int int_controller_execute_isr(int intc_id, int isr_id);

