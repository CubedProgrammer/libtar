#ifndef Included_tar_h
#define Included_tar_h
#define HEADER_SIZE 512

struct tar_header
{
    char name[101], lnk[101];
    char fpref[157];
    char user[33], group[33];
    char ver[3];
    char type;
    int mode, checksum;
    long size, modify;
    int uid, gid;
    int devmajor, devminor;
};

struct tar_header_entry
{
    char name[100];
    char mode[8];
    char uid[8], gid[8];
    char size[12];
    char modify[12];
    char checksum[7];
    char zzzzzzspace, type;
    char lnk[100];
    char zzzzzzustar[6];
    char ver[2];
    char user[32], group[32];
    char devmajor[8], devminor[8];
    char fpref[155];
};

union tar_header_data
{
    struct tar_header_entry header;
    char raw[HEADER_SIZE];
};

// header to raw and raw to header
int tar_htor(union tar_header_data *restrict dest, const struct tar_header *restrict src);
int tar_rtoh(struct tar_header *restrict dest, const union tar_header_data *restrict src);

#endif
