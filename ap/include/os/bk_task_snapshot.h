#ifndef BK_TASK_SNAPSHOT_INTERNAL_H
#define BK_TASK_SNAPSHOT_INTERNAL_H

#include <stdint.h>

enum {
  BK_TASK_SNAPSHOT_TASKS = 64,
  BK_TASK_SNAPSHOT_FRAMES = 32,
  BK_TASK_SNAPSHOT_NAME = 64,
  BK_TASK_SNAPSHOT_WORDS = 4096,
};

typedef struct bk_task_snapshot_row {
  uintptr_t identity, base, top, bottom;
  uint32_t size, count, truncated, invalid;
  char name[BK_TASK_SNAPSHOT_NAME];
  uint32_t frames[BK_TASK_SNAPSHOT_FRAMES];
} bk_task_snapshot_row_t;

typedef struct bk_task_snapshot_snapshot {
  uint32_t count, omitted, truncated, invalid;
  bk_task_snapshot_row_t rows[BK_TASK_SNAPSHOT_TASKS];
} bk_task_snapshot_t;

#ifdef __cplusplus
extern "C" {
#endif
/* Copy under the kernel lock. The caller owns the snapshot storage. */
void bk_task_snapshot_capture(bk_task_snapshot_t* snapshot);
#ifdef __cplusplus
}
#endif
#endif
