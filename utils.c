#include<stdio.h>
#include<string.h>
#include"tar.h"
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
