#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include"tar.h"
const char dl_loader[]__attribute__((section(".interp")))="/usr/lib/ld-linux-x86-64.so.2";
enum operation
{   LIST, CREATE, EXTRACT   };
void print_archive_entry(FILE *fh, const char *fmt, const struct tar_header *headerp)
{
    fprintf(fh, fmt, headerp->name, headerp->size);
}
size_t list_arch_enum_callback(void *arg, union tar_header_data *header)
{
    struct tar_header x;
    tar_rtoh(&x, header);
    print_archive_entry(stdout, "%s %li\n", &x);
    return 0;
}
size_t extract_arch_enum_callback(void *arg, union tar_header_data *header)
{
    struct tar_header x;
    long cnt = 0;
    tar_rtoh(&x, header);
    if(x.type == TAR_DIR)
    {
        if(mkdir(x.name, x.mode))
        {
            if(errno == EEXIST)
                chmod(x.name, x.mode);
            else
            {
                fprintf(stderr, "mkdir %s failed", x.name);
                perror("");
            }
        }
    }
    else
    {
        FILE *fout = fopen(x.name, "wb");
        if(fout == NULL)
        {
            fprintf(stderr, "Creating file %s", x.name);
            perror(" failed");
        }
        else
        {
            long bytes, diff = 0;
            char cbuf[TAR_HEADER_SIZE];
            FILE *fh = (FILE*)arg;
            for(; cnt < x.size; cnt += bytes)
            {
                bytes = fread(cbuf, 1, TAR_HEADER_SIZE, fh);
                if(bytes + cnt > x.size)
                    diff = cnt + bytes - x.size;
                fwrite(cbuf, 1, bytes - diff, fout);
            }
            fclose(fout);
            chmod(x.name, x.mode);
        }
    }
    return cnt;
}
void list_arch(const char *fname)
{
    FILE *fh = fopen(fname, "r");
    if(fh == NULL)
        perror("Archive file could not be opened for reading");
    else
    {
        tar_enumerate_headers(fh, list_arch_enum_callback, NULL);
        fclose(fh);
    }
}
void create_arch(const char *fname, char *entries[])
{
    FILE *fh = fopen(fname, "wb");
    if(fh == NULL)
        perror("Archive file could not be opened for writing");
    else
    {
        struct tar_header head;
        struct stat fdat;
        size_t len;
        char cbuf[TAR_HEADER_SIZE];
        FILE *fin;
        for(char **it = entries; *it != NULL; ++it)
        {
            memset(&head, 0, sizeof head);
            strcpy(head.name, *it);
            strcpy(head.ver, "00");
            stat(head.name, &fdat);
            head.size = fdat.st_size;
            head.mtime = fdat.st_mtime;
            head.uid = fdat.st_uid;
            head.gid = fdat.st_gid;
            head.mode = fdat.st_mode;
            if(S_ISREG(fdat.st_mode))
            {
                head.type = TAR_REG;
                fin = fopen(*it, "r");
            }
            else if(S_ISDIR(fdat.st_mode))
            {
                head.type = TAR_DIR;
                len = strlen(head.name);
                if(head.name[len - 1] != '/')
                    head.name[len] = '/';
            }
            tar_write(fh, &head);
            if(head.type == TAR_REG)
            {
                if(fin == NULL)
                {
                    fprintf(stderr, "Opening file %s", *it);
                    perror(" failed");
                }
                else
                {
                    len = TAR_HEADER_SIZE;
                    while(!feof(fin) && len == TAR_HEADER_SIZE)
                    {
                        len = fread(cbuf, 1, TAR_HEADER_SIZE, fin);
                        fwrite(cbuf, 1, len, fh);
                    }
                    if(len < TAR_HEADER_SIZE)
                    {
                        memset(cbuf, 0, -len & 0x1ff);
                        fwrite(cbuf, 1, -len & 0x1ff, fh);
                    }
                    fclose(fin);
                }
            }
        }
        tar_end_archive(fh);
        fclose(fh);
    }
}
void extract_arch(const char *fname)
{
    FILE *fh = fopen(fname, "r");
    if(fh == NULL)
        perror("Archive file could not be opened for reading");
    else
    {
        tar_enumerate_headers(fh, extract_arch_enum_callback, (void*)fh);
        fclose(fh);
    }
}
int main(int argl, char *argv[])
{
    int succ = 0;
    if(argl == 1)
        puts("tar https://github.com/CubedProgrammer/libtar");
    else
    {
        int optend = 1;
        enum operation op;
        char *arg, *target = NULL, nxtfile = 0;
        for(int i = 1; optend == 1 && i < argl; ++i)
        {
            arg = argv[i];
            if(arg[0] == '-')
            {
                for(char *it = arg + 1; *it != '\0'; ++it)
                {
                    switch(*it)
                    {
                        case'c':
                            op = CREATE;
                            break;
                        case'f':
                            nxtfile = 1;
                            break;
                        case't':
                            op = LIST;
                            break;
                        case'x':
                            op = EXTRACT;
                            break;
                        default:
                            fprintf(stderr, "Unrecognized option %c will be ignored.\n", *it);
                    }
                }
            }
            else if(nxtfile)
            {
                target = arg;
                nxtfile = 0;
            }
            else
                optend = i;
        }
        switch(op)
        {
            case CREATE:
                create_arch(target, argv + optend);
                break;
            case EXTRACT:
                extract_arch(target);
                break;
            case LIST:
                list_arch(target);
                break;
        }
    }
    return succ;
}
