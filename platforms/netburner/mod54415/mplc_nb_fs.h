/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPLC_NB_FS_H
#define MPLC_NB_FS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Select NOR application flash filesystem root (drive 0). */
int mplc_nb_fs_select_root(void);

/* Write upload stream to path; returns 0 on success. */
int mplc_nb_fs_write_upload(int source_fd, const char *path, long max_bytes, long *written_bytes);

/* Read entire file into malloc'd buffer; caller frees. */
int mplc_nb_fs_read_all(const char *path, uint8_t **buf, size_t *size, long max_bytes);

/* Return 0 if path exists with non-zero length. */
int mplc_nb_fs_probe_file(const char *path, long *size_bytes);

#ifdef __cplusplus
}
#endif

#endif /* MPLC_NB_FS_H */
