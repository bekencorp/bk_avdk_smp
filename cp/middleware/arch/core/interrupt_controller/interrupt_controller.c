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

#include "sdkconfig.h"
#include <common/bk_include.h>
#include <common/bk_compiler.h>
#include "interrupt_controller.h"
#include <os/mem.h>

/* global interrupt controller array */
static int_controller_t *g_int_controllers[CONFIG_ARM_INTERRUPT_CONTROLLER_CNT] = {NULL};

int_controller_t *int_controller_create(uint32_t id, uint32_t isr_capacity, void *vtor_addr)
{
        int ret = BK_OK;
        isr_item_t *int_list_entry;
        int_controller_t *int_ctrl = NULL;
        isr_func_t *vtor_entry = (isr_func_t *)vtor_addr;

        /* Validate parameter combination: only primary controller can use vtor_addr, other controllers cannot specify vtor_addr */
        if (((INT_CONTROLLER_ID_PRIMARY == id || INT_CONTROLLER_ID_SECONDARY == id) && (NULL == vtor_addr)) ||
                ((INT_CONTROLLER_ID_PRIMARY != id && INT_CONTROLLER_ID_SECONDARY != id) && (NULL != vtor_addr))) {
                ret = BK_FAIL;
                goto create_exit;  /* Invalid parameter combination, return NULL directly */
        }

        /* allocate memory for interrupt controller structure */
        int_ctrl = (int_controller_t *)os_zalloc(sizeof(int_controller_t));
        if (NULL == int_ctrl) {
                ret = BK_FAIL;
                goto create_exit;
        }

        if(INT_CONTROLLER_ID_PRIMARY == id || INT_CONTROLLER_ID_SECONDARY == id) {
                int_list_entry = (isr_item_t *)&vtor_entry[VECTOR_SYS_ENTRY_CNT];
        } else {
                int_list_entry = (isr_item_t *)os_zalloc(sizeof(isr_item_t) * isr_capacity);
                if(NULL == int_list_entry) {
                        ret = BK_FAIL;
                        goto create_exit;
                }
        }

        /* set interrupt controller properties */
        int_ctrl->id = id;
        int_ctrl->isr_capacity = isr_capacity;
        int_ctrl->int_list_entry = (isr_item_t *)int_list_entry; /* type conversion for compatibility with existing interface */
        int_ctrl->private = NULL;

        g_int_controllers[id] = int_ctrl;

create_exit:
        if(ret != BK_OK) {
                int_controller_destroy(&int_ctrl);
        }
        return int_ctrl;
}

int int_controller_destroy(int_controller_t **int_ctrl_ptr)
{
        int ret = BK_OK;

        /* parameter validation */
        if (NULL == int_ctrl_ptr || NULL == *int_ctrl_ptr) {
                return BK_FAIL;
        }

        int_controller_t *int_ctrl = *int_ctrl_ptr;

        /* free resources based on controller type */
        if (NULL != int_ctrl->int_list_entry) {
                /* only free int_list_entry for non-primary controllers,
                 * primary controller's int_list_entry is derived from vtor_entry which is not allocated here */
                if (INT_CONTROLLER_ID_PRIMARY != int_ctrl->id && INT_CONTROLLER_ID_SECONDARY != int_ctrl->id) {
                        os_free(int_ctrl->int_list_entry);
                }
                int_ctrl->int_list_entry = NULL;
        }

        /* free interrupt controller structure */
        os_free(int_ctrl);
        *int_ctrl_ptr = NULL;

        return ret;
}

int_controller_t *int_controller_get_by_id(int intc_id)
{
        /* parameter validation */
        if ((intc_id < 0) || (intc_id >= CONFIG_ARM_INTERRUPT_CONTROLLER_CNT)) {
                return NULL;
        }

        return g_int_controllers[intc_id];
}

isr_item_t* int_controller_get_isr_item_by_id(int intc_id, int isr_id)
{
        /* check if interrupt controller is valid */
        if ((intc_id < 0) || (intc_id >= CONFIG_ARM_INTERRUPT_CONTROLLER_CNT) || (NULL == g_int_controllers[intc_id])) {
                return NULL;
        }

        int_controller_t *int_ctrl = g_int_controllers[intc_id];

        /* boundary check */
        if ((isr_id < 0) || (isr_id >= int_ctrl->isr_capacity)) {
                return NULL;  /* isr_id out of range */
        }

        /* return corresponding interrupt service routine */
        return &int_ctrl->int_list_entry[isr_id];
}

int int_controller_execute_isr(int intc_id, int isr_id)
{
        /* get interrupt service routine item */
        isr_item_t* item = int_controller_get_isr_item_by_id(intc_id, isr_id);

        /* check if function pointer is valid before calling */
        if ((item) && (NULL != item->func)) {
                item->func();
                return BK_OK;
        }

        return BK_FAIL;  /* function pointer is NULL, execution failed */
}

unsigned int int_controller_get_capacity(int intc_id)
{
        if ((intc_id < 0) || (intc_id >= CONFIG_ARM_INTERRUPT_CONTROLLER_CNT)
                        || (NULL == g_int_controllers[intc_id])) {
                return 0;  /* invalid interrupt controller */
        }

        return g_int_controllers[intc_id]->isr_capacity;
}

int int_controller_get_intc_id(int_controller_t *int_ctrl)
{
        if (NULL == int_ctrl) {
                return BK_FAIL;
        }
        return int_ctrl->id;
}

int int_controller_connect(int_controller_t *int_ctrl, uint32_t int_num, isr_func_t func)
{
        /* parameter validation */
        if (NULL == int_ctrl || int_num >= int_ctrl->isr_capacity) {
                return BK_FAIL;
        }

        /* connect interrupt service routine to specified interrupt number */
        isr_item_t *ext_entry = (isr_item_t *)int_ctrl->int_list_entry;
        ext_entry[int_num].func = func;

        return BK_OK;
}

int int_controller_connect_by_intc_id(uint32_t intc_id, uint32_t int_num, isr_func_t func)
{
        int_controller_t *int_ctrl;

        /* parameter validation */
        int_ctrl = int_controller_get_by_id(intc_id);
        if(NULL == int_ctrl) {
                return BK_FAIL;
        }
        if (int_num >= int_ctrl->isr_capacity) {
                return BK_FAIL;
        }

        return int_controller_connect(int_ctrl, int_num, func);
}

int int_controller_disconnect_by_intc_id(uint32_t intc_id, uint32_t int_num)
{
        int_controller_t *int_ctrl;

        /* parameter validation */
        int_ctrl = int_controller_get_by_id(intc_id);
        if(NULL == int_ctrl) {
                return BK_FAIL;
        }
        if (int_num >= int_ctrl->isr_capacity) {
                return BK_FAIL;
        }

        return int_controller_disconnect(int_ctrl, int_num);
}

int int_controller_disconnect(int_controller_t *int_ctrl, uint32_t int_num)
{
        /* parameter validation */
        if (NULL == int_ctrl || int_num >= int_ctrl->isr_capacity) {
                return BK_FAIL;
        }

        /* disconnect interrupt service routine by setting function pointer to NULL */
        isr_item_t *ext_entry = (isr_item_t *)int_ctrl->int_list_entry;
        ext_entry[int_num].func = NULL;

        return BK_OK;
}
// eof