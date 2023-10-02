#include<stdlib.h>
#include<string.h>
#include"tar.h"
size_t tar_all_headers(FILE *src, struct tar_header **arrp)
{
    return tar_all_headers_generic((void*)src, tar_read_stdc_file_handle_helper, tar_seek_std_file_handle_helper, arrp);
}
size_t tar_all_headers_generic(void *restrict src, int(*reader)(void *restrict src, void *restrict dat, unsigned cnt), int(*seeker)(void *obj, long offset, int origin), struct tar_header **arrp)
{
    size_t cnt = 0;
    struct tar_header *arr, head;
    union tar_header_data rhead;
    long bytes = 0, skip;
    int succ = tar_read_generic(src, &rhead, reader);
    while(succ == 0)
    {
        bytes += TAR_HEADER_SIZE;
        tar_rtoh(&head, &rhead);
        skip = head.size + (-head.size & 0x1ff);
        seeker(src, skip, SEEK_CUR);
        bytes += skip;
        ++cnt;
        succ = tar_read_generic(src, &rhead, reader);
    }
    seeker(src, bytes * -1 - TAR_HEADER_SIZE, SEEK_CUR);
    arr = cnt == 0 ? NULL : malloc(cnt * sizeof(*arr));
    *arrp = arr;
    succ = tar_read_generic(src, &rhead, reader);
    while(succ == 0)
    {
        tar_rtoh(arr, &rhead);
        skip = arr->size + (-arr->size & 0x1ff);
        ++arr;
        seeker(src, skip, SEEK_CUR);
        succ = tar_read_generic(src, &rhead, reader);
    }
    return cnt;
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
