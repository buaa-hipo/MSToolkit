#ifndef __JSI_IO_H__
#define __JSI_IO_H__


#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "string"
#include "ral/backend.h"
#include "ral/section.h"


struct _jsio {

    // API
    size_t (*read)(struct _jsio *, void *buf, size_t len);

    size_t (*write)(struct _jsio *, const void *buf, size_t len);

    off_t (*tell)(struct _jsio *);

    /* number of bytes read or written */
    size_t processed_bytes;

    /* maximum single read or write chunk size */
    size_t max_processing_chunk;

    /* section for metadata */
    std::unique_ptr<pse::ral::StreamSectionInterface> stream_section;
};

typedef struct _jsio jsio;

/*
 * 将 buf 中的 len 字节写入到 r 中。
 *
 * 写入成功返回实际写入的字节数，写入失败返回 -1 。
 */
static inline size_t jsioWrite(jsio *r, const void *buf, size_t len) {
    while (len) {
        size_t bytes_to_write = (r->max_processing_chunk && r->max_processing_chunk < len) ? r->max_processing_chunk
                                                                                           : len;
        if (r->write(r, buf, bytes_to_write) == 0)
            return 0;
        buf = (char *) buf + bytes_to_write;
        len -= bytes_to_write;
        r->processed_bytes += bytes_to_write;
    }
    return 1;
}

static inline void jsioWriteString(jsio *r, std::string str) {
    size_t strSize = str.size();
    jsioWrite(r, &strSize, sizeof(size_t));
    const char *buffer = str.c_str();
    jsioWrite(r, buffer, strSize);
}

/*
 * 从 r 中读取 len 字节，并将内容保存到 buf 中。
 *
 * 读取成功返回 1 ，失败返回 0 。
 */
static inline size_t jsioRead(jsio *r, void *buf, size_t len) {
    while (len) {
        size_t bytes_to_read = (r->max_processing_chunk && r->max_processing_chunk < len) ? r->max_processing_chunk
                                                                                          : len;
        if (r->read(r, buf, bytes_to_read) == 0)
            return 0;
        buf = (char *) buf + bytes_to_read;
        len -= bytes_to_read;
        r->processed_bytes += bytes_to_read;
    }
    return 1;
}


/*
 * 返回 r 的当前偏移量。
 */
static inline off_t jsioTell(jsio *r) {
    return r->tell(r);
}

/* Returns the actual len being written.
 *
 * 将长度为 len 的内容 buf 写入到 section 中。
 *
 * 返回写入的长度 i.e. len。
 */
static size_t jsioSectionWrite(jsio *r, const void *buf, size_t len) {
    return r->stream_section->write(buf, len);
}

/*
 * 从 section 中读取 len 字节到 buf 中。
 *
 * 返回值为读取的字节数。
 */
static size_t jsioSectionRead(jsio *r, void *buf, size_t len) {
    return r->stream_section->read(buf, len);
}

/* Returns read/write position in section.
 *
 * 返回 section 当前的偏移量
 */
static off_t jsioSectionTell(jsio *r) {
    return r->stream_section->tell();
}

static void jsioInitWithSection(jsio *r, std::unique_ptr<pse::ral::StreamSectionInterface> &&stream_section) {
    r->read = jsioSectionRead;
    r->write = jsioSectionWrite;
    r->tell = jsioTell;
    r->max_processing_chunk = 0;
    r->processed_bytes = 0;
    r->stream_section = std::move(stream_section);
}

#endif

