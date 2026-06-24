/*
 * Copyright (c) 2026 Adam G. Sweeney <agsweeney@gmail.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <http.h>
#include <httppost.h>
#include <iosys.h>
#include <nbrtos.h>
#include <hal.h>
#include <system.h>
#include <stdio.h>
#include <string.h>

#include "hal_netburner_config.h"
#include "mplc_nb_fs.h"

namespace {

static int g_upload_result = -1;
static long g_upload_bytes = 0;

static void send_upload_page(int sock, HTTP_Request &req)
{
    (void)req;
    SendHTMLHeader(sock);
    writestring(sock,
                "<html><head><title>MPLC Update</title></head><body>"
                "<h2>MPLC Program Upload</h2>"
                "<p>Select a compiled <code>.mplc</code> package file to store as "
                "<code>");
    writestring(sock, MPLC_NB_PACKAGE_FILE_PATH);
    writestring(sock,
                "</code> in flash.</p>"
                "<form method=\"POST\" action=\"mplc_upload.html\" enctype=\"multipart/form-data\">"
                "<input type=\"file\" name=\"plcfile\" accept=\".mplc\" required>"
                "<br><br><button type=\"submit\">Upload PLC Program</button>"
                "</form>"
                "<p>After upload, reboot to boot the new program.</p>"
                "</body></html>");
}

static void send_upload_result_page(int sock, int result, long bytes)
{
    SendHTMLHeader(sock);
    writestring(sock, "<html><head><title>MPLC Upload Result</title></head><body>");
    if (result == 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Upload complete: %ld bytes written to %s", bytes, MPLC_NB_PACKAGE_FILE_PATH);
        writestring(sock, "<h2>Upload Complete</h2><p>");
        writestring(sock, msg);
        writestring(sock, "</p>");
        writestring(sock,
                    "<form method=\"POST\" action=\"mplc_reboot.html\">"
                    "<button type=\"submit\">Reboot Now</button>"
                    "</form>");
    } else {
        char msg[96];
        snprintf(msg, sizeof(msg), "Upload failed (error %d).", result);
        writestring(sock, "<h2>Upload Failed</h2><p>");
        writestring(sock, msg);
        writestring(sock, "</p>");
    }
    writestring(sock,
                "<p><a href=\"mplc_update.html\">Back to upload page</a></p>"
                "</body></html>");
}

static int mplc_update_page_handler(int sock, HTTP_Request &req)
{
    send_upload_page(sock, req);
    return 1;
}

static int mplc_upload_post_handler(int sock, PostEvents event, const char *pName, const char *pValue)
{
    switch (event) {
    case eStartingPost:
        g_upload_result = -1;
        g_upload_bytes = 0;
        break;

    case eVariable:
        break;

    case eFile: {
        FilePostStruct *fps = (FilePostStruct *)pValue;
        if (!fps || fps->fd <= 0) {
            g_upload_result = -16;
            break;
        }
        g_upload_result = mplc_nb_fs_write_upload(
            fps->fd,
            MPLC_NB_PACKAGE_FILE_PATH,
            (long)MPLC_NB_PACKAGE_FILE_MAX_BYTES,
            &g_upload_bytes);
        close(fps->fd);
        break;
    }

    case eEndOfPost:
        if (g_upload_result == -1) {
            g_upload_result = -16;
        }
        send_upload_result_page(sock, g_upload_result, g_upload_bytes);
        break;
    }
    return 0;
}

static int mplc_reboot_post_handler(int sock, PostEvents event, const char *pName, const char *pValue)
{
    (void)pName;
    (void)pValue;
    if (event == eEndOfPost) {
        SendHTMLHeader(sock);
        writestring(sock,
                    "<html><head><title>Rebooting</title></head><body>"
                    "<h2>Rebooting device...</h2>"
                    "<p>The new PLC package will boot after restart.</p>"
                    "</body></html>");
        close(sock);
        OSTimeDly(2 * TICKS_PER_SECOND);
        ForceReboot();
    }
    return 0;
}

CallBackFunctionPageHandler g_mplc_update_page("mplc_update.html", mplc_update_page_handler, tGet, 0, true);
HtmlPostVariableListCallback g_mplc_upload_post("mplc_upload.html", mplc_upload_post_handler);
HtmlPostVariableListCallback g_mplc_reboot_post("mplc_reboot.html", mplc_reboot_post_handler);

} // namespace
