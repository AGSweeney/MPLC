/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mplc_nb_fs.h"

#include <file/effsstd.h>
#include <file/fsf.h>
#include <iosys.h>
#include <stdlib.h>
#include <string.h>

#define MPLC_NB_FS_DRIVE 0

int mplc_nb_fs_select_root(void)
{
    if (fs_chdrive(MPLC_NB_FS_DRIVE) != FS_NO_ERROR) {
        return -1;
    }
    if (fs_chdir("\\") != FS_NO_ERROR) {
        return -2;
    }
    return 0;
}

int mplc_nb_fs_write_upload(int source_fd, const char *path, long max_bytes, long *written_bytes)
{
    FS_FILE *fp;
    char buffer[4096];
    long total = 0;
    int rv;

    if (written_bytes) {
        *written_bytes = 0;
    }
    if (!path || path[0] == '\0' || source_fd < 0) {
        return -1;
    }
    if (mplc_nb_fs_select_root() != 0) {
        return -10;
    }

    fp = fs_open(path, "w");
    if (fp == NULL) {
        return -11;
    }

    while ((rv = read(source_fd, buffer, sizeof(buffer))) > 0) {
        if (fs_write(buffer, 1U, (unsigned long)rv, fp) != (unsigned long)rv) {
            fs_close(fp);
            return -12;
        }
        total += rv;
        if (total > max_bytes) {
            fs_close(fp);
            return -13;
        }
    }

    fs_flush(fp);
    fs_close(fp);

    if (rv < 0) {
        return -14;
    }
    if (total <= 0) {
        return -15;
    }

    if (written_bytes) {
        *written_bytes = total;
    }
    return 0;
}

int mplc_nb_fs_probe_file(const char *path, long *size_bytes)
{
    long len;

    if (!path || path[0] == '\0') {
        return -1;
    }
    if (mplc_nb_fs_select_root() != 0) {
        return -2;
    }

    len = fs_filelength(path);
    if (len <= 0L) {
        return -3;
    }
    if (size_bytes) {
        *size_bytes = len;
    }
    return 0;
}

int mplc_nb_fs_read_all(const char *path, uint8_t **buf, size_t *size, long max_bytes)
{
    FS_FILE *fp;
    long file_size;
    size_t nread;
    uint8_t *tmp;

    if (!buf || !size || !path || path[0] == '\0') {
        return -1;
    }
    *buf = NULL;
    *size = 0U;

    if (mplc_nb_fs_select_root() != 0) {
        return -2;
    }

    fp = fs_open(path, "r");
    if (fp == NULL) {
        return -3;
    }

    if (fs_seek(fp, 0L, SEEK_END) != FS_NO_ERROR) {
        fs_close(fp);
        return -4;
    }
    file_size = fs_tell(fp);
    if (file_size <= 0L) {
        fs_close(fp);
        return -5;
    }
    if (file_size > max_bytes) {
        fs_close(fp);
        return -6;
    }
    if (fs_seek(fp, 0L, SEEK_SET) != FS_NO_ERROR) {
        fs_close(fp);
        return -7;
    }

    tmp = (uint8_t *)malloc((size_t)file_size);
    if (!tmp) {
        fs_close(fp);
        return -8;
    }

    nread = fs_read(tmp, 1U, (unsigned long)file_size, fp);
    fs_close(fp);
    if (nread != (size_t)file_size) {
        free(tmp);
        return -9;
    }

    *buf = tmp;
    *size = nread;
    return 0;
}
