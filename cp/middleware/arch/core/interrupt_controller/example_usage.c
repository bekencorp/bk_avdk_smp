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

#include "interrupt_controller.h"
#include <stdio.h>

/* example interrupt service routines */
void example_isr_0(void)
{
        printf("Executing interrupt service routine 0\n");
}

void example_isr_5(void)
{
        printf("Executing interrupt service routine 5\n");
}

void example_isr_10(void)
{
        printf("Executing interrupt service routine 10\n");
}

int main(void)
{
        /* 1. create interrupt controller PRIMARY */
        int_controller_t *nvic0 = int_controller_create(INT_CONTROLLER_ID_PRIMARY, 16, NULL); /* primary controller with capacity 16 */
        if (NULL == nvic0) {
                printf("Failed to create primary interrupt controller\n");
                return BK_FAIL;
        }
        printf("Primary interrupt controller created successfully with capacity: %u\n",
               int_controller_get_capacity(INT_CONTROLLER_ID_PRIMARY));

        /* register primary interrupt controller */
        if (int_controller_register(INT_CONTROLLER_ID_PRIMARY, nvic0) != BK_OK) {
                printf("Failed to register primary interrupt controller\n");
                int_controller_destroy(&nvic0);
                return BK_FAIL;
        }

        /* 2. create interrupt controller SECONDARY */
        int_controller_t *nvic1 = int_controller_create(INT_CONTROLLER_ID_SECONDARY, 8, NULL); /* secondary controller with capacity 8 */
        if (NULL == nvic1) {
                printf("Failed to create secondary interrupt controller\n");
                int_controller_destroy(&nvic0);
                return BK_FAIL;
        }
        printf("Secondary interrupt controller created successfully with capacity: %u\n",
               int_controller_get_capacity(INT_CONTROLLER_ID_SECONDARY));

        /* register secondary interrupt controller */
        if (int_controller_register(INT_CONTROLLER_ID_SECONDARY, nvic1) != BK_OK) {
                printf("Failed to register secondary interrupt controller\n");
                int_controller_destroy(&nvic0);
                int_controller_destroy(&nvic1);
                return BK_FAIL;
        }

        /* 3. connect interrupt service routines */
        if (int_controller_connect(nvic0, 0, example_isr_0) != BK_OK) {
                printf("Failed to connect ISR 0 to primary controller\n");
        }

        if (int_controller_connect(nvic0, 5, example_isr_5) != BK_OK) {
                printf("Failed to connect ISR 5 to primary controller\n");
        }

        if (int_controller_connect(nvic1, 2, example_isr_10) != BK_OK) {
                printf("Failed to connect ISR 10 to secondary controller\n");
        }

        /* 4. execute interrupt service routines via controller IDs */
        printf("\n--- Testing ISR execution ---\n");
        int_controller_execute_isr(INT_CONTROLLER_ID_PRIMARY, 0);  /* execute interrupt 0 of primary controller */
        int_controller_execute_isr(INT_CONTROLLER_ID_PRIMARY, 5);  /* execute interrupt 5 of primary controller */
        int_controller_execute_isr(INT_CONTROLLER_ID_SECONDARY, 2);  /* execute interrupt 2 of secondary controller */

        /* test unconnected interrupt */
        int result = int_controller_execute_isr(INT_CONTROLLER_ID_PRIMARY, 1);
        if (result != BK_OK) {
                printf("Expected failure: ISR 1 is not connected to primary controller\n");
        }

        /* 5. disconnect interrupt */
        int_controller_disconnect(nvic0, 0);
        result = int_controller_execute_isr(INT_CONTROLLER_ID_PRIMARY, 0);
        if (result != BK_OK) {
                printf("Expected failure: ISR 0 was disconnected from primary controller\n");
        }

        /* Test ISR 5 even after other ISRs are disconnected */
        int_controller_execute_isr(INT_CONTROLLER_ID_PRIMARY, 5);

        printf("\nExample application completed\n");

        /* Clean up resources using int_controller_destroy to free all resources allocated by int_controller_create */
        if (int_controller_destroy(&nvic0) != BK_OK) {
                printf("Failed to destroy primary interrupt controller\n");
        }

        if (int_controller_destroy(&nvic1) != BK_OK) {
                printf("Failed to destroy secondary interrupt controller\n");
        }

        /* Verify controllers are properly destroyed */
        if (nvic0 == NULL && nvic1 == NULL) {
                printf("All interrupt controllers destroyed successfully\n");
        }

        return BK_OK;
}