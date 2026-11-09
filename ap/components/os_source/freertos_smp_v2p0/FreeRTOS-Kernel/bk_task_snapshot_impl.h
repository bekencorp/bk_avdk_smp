/* Included once by tasks.c, after the kernel definitions.
 * Its private TCB/list access must use the same kernel lock as task movement
 * and deletion. Never allocate, print, or retain a TCB for later use here. */
#include "os/bk_task_snapshot.h"

#include <string.h>

extern int code_addr_is_valid(uint32_t address);

static void bk_task_snapshot_copy(bk_task_snapshot_t* snapshot,
                                  TCB_t* task) {
  for (uint32_t i = 0; i < snapshot->count; ++i)
    if (snapshot->rows[i].identity == (uintptr_t)task) return;
  if (snapshot->count == BK_TASK_SNAPSHOT_TASKS) {
    ++snapshot->omitted;
    return;
  }
  bk_task_snapshot_row_t* row = &snapshot->rows[snapshot->count++];
  memset(row, 0, sizeof(*row));
  row->identity = (uintptr_t)task;
  if (task == NULL || (uintptr_t)task % sizeof(void*) != 0u) {
    row->invalid = 1u;
    ++snapshot->invalid;
    return;
  }
#if configBK_FREERTOS && CONFIG_USE_STATIC_TASK_NAME
  const char* name = task->pcTaskName != NULL ? task->pcTaskName : "<unnamed>";
  const size_t name_capacity = SIZE_MAX;
#else
  const char* name = task->pcTaskName;
  const size_t name_capacity = sizeof(task->pcTaskName);
#endif
  const size_t name_size = name_capacity < sizeof(row->name) - 1u
                               ? name_capacity
                               : sizeof(row->name) - 1u;
  size_t copied = 0u;
  while (copied < name_size && name[copied] != '\0') {
    row->name[copied] = name[copied];
    ++copied;
  }
  if (copied == name_size &&
      (copied == name_capacity || name[copied] != '\0')) {
    row->truncated = 1u;
    ++snapshot->truncated;
  }
  row->base = (uintptr_t)task->pxStack;
  row->top = (uintptr_t)task->pxTopOfStack;
  row->size = task->ulStackSize;
  if (row->base > UINTPTR_MAX - row->size) {
    row->invalid = 1u;
    ++snapshot->invalid;
    return;
  }
  row->bottom = row->base + row->size;
  if (row->base == 0u || row->size == 0u ||
      row->base % sizeof(uint32_t) != 0u || row->top < row->base ||
      row->top > row->bottom || row->top % sizeof(uint32_t) != 0u ||
      row->size % sizeof(uint32_t) != 0u) {
    row->invalid = 1u;
    ++snapshot->invalid;
    return;
  }
  uintptr_t address = row->top;
  uint32_t words = 0u;
  while (address < row->bottom && words < BK_TASK_SNAPSHOT_WORDS &&
         row->count < BK_TASK_SNAPSHOT_FRAMES) {
    const uint32_t value = *(const uint32_t*)address;
    if (code_addr_is_valid(value)) row->frames[row->count++] = value & ~1u;
    address += sizeof(uint32_t);
    ++words;
  }
  if (address < row->bottom && row->truncated == 0u) {
    row->truncated = 1u;
    ++snapshot->truncated;
  }
}

static void bk_task_snapshot_list(bk_task_snapshot_t* snapshot,
                                  const List_t* list) {
  const configLIST_VOLATILE ListItem_t* item = list->xListEnd.pxNext;
  for (UBaseType_t i = 0u;
       i < list->uxNumberOfItems && item != (const void*)&list->xListEnd;
       ++i, item = item->pxNext) {
    if (item == NULL || (uintptr_t)item % sizeof(void*) != 0u) {
      ++snapshot->invalid;
      break;
    }
    bk_task_snapshot_copy(snapshot, (TCB_t*)item->pvOwner);
  }
}

void bk_task_snapshot_capture(bk_task_snapshot_t* snapshot) {
  if (snapshot == NULL) return;
  snapshot->count = snapshot->omitted = snapshot->truncated =
      snapshot->invalid = 0u;
  prvENTER_CRITICAL(&xKernelLock);
  for (unsigned i = 0u; i < configMAX_PRIORITIES; ++i)
    bk_task_snapshot_list(snapshot, &pxReadyTasksLists[i]);
  bk_task_snapshot_list(snapshot, &xDelayedTaskList1);
  bk_task_snapshot_list(snapshot, &xDelayedTaskList2);
  for (unsigned core = 0u; core < portNUM_PROCESSORS; ++core)
    bk_task_snapshot_list(snapshot, &xPendingReadyList[core]);
#if INCLUDE_vTaskSuspend == 1
  bk_task_snapshot_list(snapshot, &xSuspendedTaskList);
#endif
  prvEXIT_CRITICAL(&xKernelLock);
}
