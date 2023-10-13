#include<errno.h>
#include<string.h>
#include"tar.h"
int tar_read_stdc_file_handle_helper(void *restrict src, void *restrict dat, unsigned cnt)
{
    return fread(dat, 1, cnt, (FILE*)src);
}
int tar_write_stdc_file_handle_helper(void *restrict dest, const void *restrict dat, unsigned cnt)
{
    return fwrite(dat, 1, cnt, (FILE*)dest);
}
int tar_seek_std_file_handle_helper(void *obj, long offset, int origin)
{
    return fseek((FILE*)obj, offset, origin);
}
int tar_read(FILE *src, struct tar_header *head)
{
    union tar_header_data dat;
    int succ = tar_read_raw(src, &dat);
    if(succ == 0)
        succ = tar_rtoh(head, &dat);
    return succ;
}
int tar_read_raw(FILE *src, union tar_header_data *dat)
{
    return tar_read_generic((void*)src, dat, tar_read_stdc_file_handle_helper);
}
int tar_read_generic(void *restrict src, union tar_header_data *restrict dat, int(*reader)(void *restrict src, void *restrict dat, unsigned cnt))
{
    int succ = -1;
    unsigned tot = 0;
    int bcnt;
    for(tot = bcnt = reader(src, dat->raw, TAR_HEADER_SIZE); bcnt > 0 && tot < TAR_HEADER_SIZE; tot += bcnt = reader(src, dat->raw + tot, TAR_HEADER_SIZE - tot));
    if(tot == TAR_HEADER_SIZE)
    {
        for(unsigned i = 0; succ == -1 && i < tot; ++i)
            succ = (!dat->raw[i]) * -1;
    }
    else
        errno = ENODATA;
    return succ;
}
int tar_write(FILE *dest, const struct tar_header *head)
{
    union tar_header_data dat;
    int succ = tar_htor(&dat, head);
    if(succ == 0)
        succ = tar_write_raw(dest, &dat);
    return succ;
}
int tar_write_raw(FILE *dest, const union tar_header_data *dat)
{
    return tar_write_generic(dest, dat, tar_write_stdc_file_handle_helper);
}
int tar_write_generic(void *restrict dest, const union tar_header_data *restrict dat, int(*writer)(void *restrict dest, const void *restrict dat, unsigned cnt))
{
    return writer(dest, dat->raw, TAR_HEADER_SIZE) < TAR_HEADER_SIZE;
}
int tar_end_archive(FILE *fh)
{
    return tar_end_archive_generic(fh, tar_write_stdc_file_handle_helper);
}
int tar_end_archive_generic(void *restrict dest, int(*writer)(void *restrict dest, const void *restrict dat, unsigned cnt))
{
    char cbuf[TAR_HEADER_SIZE];
    memset(cbuf, 0, sizeof cbuf);
    return writer(dest, cbuf, sizeof cbuf) + writer(dest, cbuf, sizeof cbuf) < TAR_HEADER_SIZE * 2;
}
int tar_htor(union tar_header_data *restrict dest, const struct tar_header *restrict src)
{
    int checksum = 0;
    unsigned namlen = strlen(src->name);
    memset(dest->raw, 0, TAR_HEADER_SIZE);
    dest->header.type = src->type;
    for(unsigned i = 0; i < sizeof(dest->header.mode) - 1; ++i)dest->header.mode[i] = (src->mode >> (sizeof(dest->header.mode) - 2 - i) * 3 & 7) + '0';
    for(unsigned i = 0; i < sizeof(dest->header.size) - 1; ++i)dest->header.size[i] = (src->size >> (sizeof(dest->header.size) - 2 - i) * 3 & 7) + '0';
    for(unsigned i = 0; i < sizeof(dest->header.uid) - 1; ++i)dest->header.uid[i] = (src->uid >> (sizeof(dest->header.uid) - 2 - i) * 3 & 7) + '0';
    for(unsigned i = 0; i < sizeof(dest->header.gid) - 1; ++i)dest->header.gid[i] = (src->gid >> (sizeof(dest->header.gid) - 2 - i) * 3 & 7) + '0';
    for(unsigned i = 0; i < sizeof(dest->header.mtime) - 1; ++i)dest->header.mtime[i] = (src->mtime >> (sizeof(dest->header.mtime) - 2 - i) * 3 & 7) + '0';
    for(unsigned i = 0; i < sizeof(dest->header.devmajor) - 1; ++i)dest->header.devmajor[i] = (src->devmajor >> (sizeof(dest->header.devmajor) - 2 - i) * 3 & 7) + '0';
    for(unsigned i = 0; i < sizeof(dest->header.devminor) - 1; ++i)dest->header.devminor[i] = (src->devminor >> (sizeof(dest->header.devminor) - 2 - i) * 3 & 7) + '0';
    if(namlen > sizeof(dest->header.name))
    {
        memcpy(dest->header.name, src->name + namlen - sizeof(dest->header.name), sizeof(dest->header.name));
        memcpy(dest->header.fpref, src->name, namlen - sizeof(dest->header.name));
    }
    else
        memcpy(dest->header.name, src->name, namlen);
    memcpy(dest->header.lnk, src->lnk, strlen(src->lnk));
    memcpy(dest->header.user, src->user, strlen(src->user));
    memcpy(dest->header.group, src->group, strlen(src->group));
    memcpy(dest->header.ver, src->ver, strlen(src->ver));
    strcpy(dest->header.zzzzzzustar, "ustar");
    dest->header.zzzzzzspace = ' ';
    memset(dest->header.checksum, ' ', sizeof(dest->header.checksum));
    for(int i = 0; i < TAR_HEADER_SIZE; ++i)
        checksum += (unsigned)dest->raw[i] & 0xff;
    for(unsigned i = 0; i < sizeof(dest->header.checksum) - 1; ++i)dest->header.checksum[i] = (checksum >> (sizeof(dest->header.checksum) - 2 - i) * 3 & 7) + '0';
    dest->header.checksum[sizeof(dest->header.checksum) - 1] = '\0';
    return 0;
}
int tar_rtoh(struct tar_header *restrict dest, const union tar_header_data *restrict src)
{
    unsigned preflen = strlen(src->header.fpref);
    dest->type = src->header.type;
    dest->mode = 0;
    for(unsigned i = 0; i < sizeof(src->header.mode) - 1; ++i)dest->mode = dest->mode << 3 | src->header.mode[i] - '0';
    dest->size = 0;
    for(unsigned i = 0; i < sizeof(src->header.size) - 1; ++i)dest->size = dest->size << 3 | src->header.size[i] - '0';
    dest->uid = 0;
    for(unsigned i = 0; i < sizeof(src->header.uid) - 1; ++i)dest->uid = dest->uid << 3 | src->header.uid[i] - '0';
    dest->gid = 0;
    for(unsigned i = 0; i < sizeof(src->header.gid) - 1; ++i)dest->gid = dest->gid << 3 | src->header.gid[i] - '0';
    dest->mtime = 0;
    for(unsigned i = 0; i < sizeof(src->header.mtime) - 1; ++i)dest->mtime = dest->mtime << 3 | src->header.mtime[i] - '0';
    dest->devmajor = 0;
    for(unsigned i = 0; i < sizeof(src->header.devmajor) - 1; ++i)dest->devmajor = dest->devmajor << 3 | src->header.devmajor[i] - '0';
    dest->devminor = 0;
    for(unsigned i = 0; i < sizeof(src->header.devminor) - 1; ++i)dest->devminor = dest->devminor << 3 | src->header.devminor[i] - '0';
    if(preflen > 0)
        memcpy(dest->name, src->header.fpref, preflen);
    memcpy(dest->name + preflen, src->header.name, sizeof(src->header.name));
    memcpy(dest->lnk, src->header.lnk, sizeof(src->header.lnk));
    memcpy(dest->user, src->header.user, sizeof(src->header.user));
    memcpy(dest->group, src->header.group, sizeof(src->header.group));
    memcpy(dest->ver, src->header.ver, sizeof(src->header.ver));
    dest->name[sizeof(src->header.name)] = '\0';
    dest->lnk[sizeof(src->header.lnk)] = '\0';
    dest->user[sizeof(src->header.user)] = '\0';
    dest->group[sizeof(src->header.group)] = '\0';
    dest->ver[sizeof(src->header.ver)] = '\0';
    return 0;
}
