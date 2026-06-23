#include "mplc/loader.h"
#include "mplc_endian.h"
#include <string.h>

static const uint8_t *section_data(const uint8_t *base, const mplc_section_header_t *sections,
                                   uint32_t count, mplc_section_type_t type, uint32_t *out_size)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        const uint8_t *sh = (const uint8_t *)&sections[i];
        uint32_t stype = mplc_read_le32(sh);
        uint32_t off = mplc_read_le32(sh + 4);
        uint32_t sz = mplc_read_le32(sh + 8);
        if (stype == (uint32_t)type) {
            if (out_size) {
                *out_size = sz;
            }
            return base + off;
        }
    }
    if (out_size) {
        *out_size = 0;
    }
    return NULL;
}

bool mplc_loader_validate(const uint8_t *data, size_t size)
{
    if (!data || size < sizeof(mplc_pkg_header_t)) {
        return false;
    }
    return mplc_read_le32(data) == MPLC_MAGIC &&
           mplc_read_le16(data + 4) == MPLC_ABI_VERSION;
}

int mplc_loader_parse(const uint8_t *data, size_t size, mplc_loaded_pkg_t *out)
{
    const mplc_pkg_header_t *hdr;
    const mplc_section_header_t *sections;
    uint32_t section_count;
    uint32_t i;
    uint32_t sec_size;

    if (!data || !out || size < sizeof(mplc_pkg_header_t)) {
        return -1;
    }

    memset(out, 0, sizeof(*out));

    if (!mplc_loader_validate(data, size)) {
        return -2;
    }

    section_count = mplc_read_le32(data + 8);
    if (size < sizeof(mplc_pkg_header_t) + section_count * sizeof(mplc_section_header_t)) {
        return -3;
    }

    hdr = (const mplc_pkg_header_t *)data;
    sections = (const mplc_section_header_t *)(data + sizeof(mplc_pkg_header_t));
    out->image = data;
    out->image_size = size;
    out->header = hdr;

    for (i = 0; i < section_count; i++) {
        const uint8_t *sh = (const uint8_t *)&sections[i];
        uint32_t off = mplc_read_le32(sh + 4);
        uint32_t sec_sz = mplc_read_le32(sh + 8);
        if (off + sec_sz > size) {
            return -4;
        }
    }

    out->code_base = section_data(data, sections, section_count, MPLC_SECTION_CODE, &sec_size);
    out->code_size = sec_size;

    out->data_base = section_data(data, sections, section_count, MPLC_SECTION_DATA, &sec_size);
    out->data_size = sec_size;

    out->pous = (const mplc_pou_desc_t *)section_data(data, sections, section_count,
                                                      MPLC_SECTION_SYMTAB, &sec_size);
    if (out->pous && sec_size >= sizeof(mplc_pou_desc_t)) {
        out->pou_count = sec_size / (uint32_t)sizeof(mplc_pou_desc_t);
    }

    out->tasks = (const mplc_task_desc_t *)section_data(data, sections, section_count,
                                                        MPLC_SECTION_TASKS, &sec_size);
    if (out->tasks && sec_size >= sizeof(mplc_task_desc_t)) {
        out->task_count = sec_size / (uint32_t)sizeof(mplc_task_desc_t);
    }

    out->io_map = (const mplc_io_entry_t *)section_data(data, sections, section_count,
                                                        MPLC_SECTION_IOMAP, &sec_size);
    if (out->io_map && sec_size >= sizeof(mplc_io_entry_t)) {
        out->io_count = sec_size / (uint32_t)sizeof(mplc_io_entry_t);
    }

    out->fb_instances = (const mplc_fb_instance_t *)section_data(data, sections, section_count,
                                                                 MPLC_SECTION_FBINST, &sec_size);
    if (out->fb_instances && sec_size >= sizeof(mplc_fb_instance_t)) {
        out->fb_count = sec_size / (uint32_t)sizeof(mplc_fb_instance_t);
    }

    out->sfc_steps = (const mplc_sfc_step_t *)section_data(data, sections, section_count,
                                                           MPLC_SECTION_SFC, &sec_size);
    if (out->sfc_steps && sec_size >= sizeof(mplc_sfc_step_t)) {
        out->sfc_step_count = sec_size / (uint32_t)sizeof(mplc_sfc_step_t);
    }

    return 0;
}
