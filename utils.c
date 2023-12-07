#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include"tar.h"
int tar_enumerate_headers(FILE *src, size_t(*callback)(void *arg, union tar_header_data *header), void *arg)
{
    return tar_enumerate_headers_generic((void*)src, tar_read_stdc_file_handle_helper, tar_seek_std_file_handle_helper, callback, arg);
}
int tar_enumerate_headers_generic(void *restrict src, int(*reader)(void *restrict src, void *restrict dat, unsigned cnt), int(*seeker)(void *obj, long offset, int origin), size_t(*callback)(void *arg, union tar_header_data *header), void *arg)
{
    union tar_header_data rhead;
    long skip, size;
    int succ = tar_read_generic(src, &rhead, reader);
    while(succ == 0)
    {
        size = strtol(rhead.header.size, NULL, 8);
        skip = size + (-size & 0x1ff) - callback(arg, &rhead);
        if(seeker(src, skip, SEEK_CUR))
        {
            for(long cnt, tot= 0; tot < skip; tot += cnt)
            {
                cnt = TAR_HEADER_SIZE;
                if(skip < tot + cnt)
                    cnt = skip - tot;
                cnt = reader(src, rhead.raw, cnt);
            }
        }
        succ = tar_read_generic(src, &rhead, reader);
    }
    return errno != 0;
}
int tar_read_content(FILE *src, struct tar_header *head, void *buf, size_t bufsz)
{
    int succ = tar_read(src, head);
    if(succ == 0)
    {
        if(bufsz > head->size)
            bufsz = head->size;
        succ = fread(buf, 1, bufsz, src) < bufsz || fseek(src, -bufsz & 0x1ff, SEEK_CUR);
    }
    return succ;
}
int tar_write_content(FILE *dest, const struct tar_header *head, const void *buf, size_t bufsz)
{
    int succ = tar_write(dest, head);
    if(succ == 0)
        succ = fwrite(buf, 1, bufsz, dest) < bufsz;
    if(succ == 0)
    {
        char cbuf[TAR_HEADER_SIZE];
        memset(cbuf, 0, TAR_HEADER_SIZE);
        bufsz = -bufsz & 0x1ff;
        succ = fwrite(cbuf, 1, bufsz, dest) < bufsz;
    }
    return succ;
}
