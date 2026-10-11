/**
 ******************************************************************************
 * @file    rm_log.c
 * @brief   RM_LOG storage definition.  No entry list needed — entries are
 *          auto-registered at runtime on first RMLOG(name, val) call.
 *
 *          To add log points: just write RMLOG(name, value) anywhere.
 *          No need to edit this file.
 ******************************************************************************
 */

#include "../Inc/rm_log.h"

RMLogArea rm_log_area = {
    .header = {
        .magic        = RM_LOG_MAGIC,
        .version      = RM_LOG_VERSION,
        .entry_count  = 0,               /* grows at runtime */
        .frame_count  = RM_LOG_FRAME_COUNT,
        .max_name_len = RM_LOG_MAX_NAME_LEN,
    },
    .entries = {{{0}}},
};

void rm_log_init(void) {
    /* All data statically initialized. No-op. */
    (void)rm_log_area;
}
