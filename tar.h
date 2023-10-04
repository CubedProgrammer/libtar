#ifndef Included_tar_h
#define Included_tar_h
#include<stdio.h>
#define TAR_HEADER_SIZE 512
#define TAR_REG '0'
#define TAR_LNK '1'
#define TAR_SYM '2'
#define TAR_CHR '3'
#define TAR_BLK '4'
#define TAR_DIR '5'
#define TAR_PIP '6'

struct tar_header
{
    char name[257], lnk[101];
    char user[33], group[33];
    char ver[3];
    char type;
    int mode;
    long size, mtime;
    int uid, gid;
    int devmajor, devminor;
};

struct tar_header_entry
{
    char name[100];
    char mode[8];
    char uid[8], gid[8];
    char size[12];
    char mtime[12];
    char checksum[7];
    char zzzzzzspace, type;
    char lnk[100];
    char zzzzzzustar[6];
    char ver[2];
    char user[32], group[32];
    char devmajor[8], devminor[8];
    char fpref[155];
    char blank[12];
};

union tar_header_data
{
    struct tar_header_entry header;
    char raw[TAR_HEADER_SIZE];
};

int tar_read_content(FILE *src, struct tar_header *head, void *buf, size_t bufsz);
int tar_read(FILE *src, struct tar_header *head);
int tar_read_raw(FILE *src, union tar_header_data *dat);
int tar_read_generic(void *restrict src, union tar_header_data *restrict dat, int(*reader)(void *restrict src, void *restrict dat, unsigned cnt));
int tar_write_content(FILE *dest, const struct tar_header *head, const void *buf, size_t bufsz);
int tar_write(FILE *dest, const struct tar_header *head);
int tar_write_raw(FILE *dest, const union tar_header_data *dat);
int tar_write_generic(void *restrict dest, const union tar_header_data *restrict dat, int(*writeer)(void *restrict dest, const void *restrict dat, unsigned cnt));
int tar_end_archive(FILE *fh);
int tar_end_archive_generic(void *restrict dest, int(*writer)(void *restrict dest, const void *restrict dat, unsigned cnt));
int tar_enumerate_headers(FILE *src, size_t(*callback)(void *arg, union tar_header_data *header), void *arg);
int tar_enumerate_headers_generic(void *restrict src, int(*reader)(void *restrict src, void *restrict dat, unsigned cnt), int(*seeker)(void *obj, long offset, int origin), size_t(*callback)(void *arg, union tar_header_data *header), void *arg);
// header to raw and raw to header
int tar_htor(union tar_header_data *restrict dest, const struct tar_header *restrict src);
int tar_rtoh(struct tar_header *restrict dest, const union tar_header_data *restrict src);
int tar_read_stdc_file_handle_helper(void *restrict src, void *restrict dat, unsigned cnt);
int tar_write_stdc_file_handle_helper(void *restrict dest, const void *restrict dat, unsigned cnt);
int tar_seek_std_file_handle_helper(void *obj, long offset, int origin);

#endif
