#include<dirent.h>
#include<errno.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/sysmacros.h>
#include"tar.h"
const char dl_loader[]__attribute__((section(".interp")))="/usr/lib/ld-linux-x86-64.so.2";
enum operation
{   APPEND, CONCAT, LIST, CREATE, EXTRACT   };
char self_or_parent_directory(const char *name)
{
    return name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'));
}
void recursive_directory_iterator(const char *dname, void(*callback)(void *arg, const char *dname), void *arg)
{
    DIR *handles[250];
    char path[3601];
    struct dirent *en;
    unsigned depth = 1, pathlen = strlen(dname);
    strcpy(path, dname);
    handles[0] = opendir(dname);
    path[pathlen] = '/';
    ++pathlen;
    while(depth)
    {
        errno = 0;
        en = readdir(handles[depth - 1]);
        if(en == NULL)
        {
            if(errno)
                perror("readdir failed");
            closedir(handles[depth - 1]);
            --depth;
            for(path[--pathlen] = '\0'; pathlen > 0 && path[pathlen] != '/'; --pathlen);
            ++pathlen;
        }
        else if(!self_or_parent_directory(en->d_name))
        {
            strcpy(path + pathlen, en->d_name);
            callback(arg, path);
            if(en->d_type == DT_DIR)
            {
                pathlen += strlen(en->d_name);
                handles[depth] = opendir(path);
                path[pathlen] = '/';
                ++pathlen;
                ++depth;
            }
        }
    }
}
size_t append_arch_enum_callback(void *arg, union tar_header_data *header)
{
    return 0;
}
size_t list_arch_enum_callback(void *arg, union tar_header_data *header)
{
    struct tar_header x;
    tar_rtoh(&x, header);
    if(arg == NULL)
        printf("%s\n", x.name);
    else
    {
        int shiftcnt, pos;
        char modestr[11] = "----------";
        char rwx[4] = "rwx";
        for(int i = 0; i < 3; ++i)
        {
            for(int j = 0; j < 3; ++j)
            {
                pos = i * 3 + j;
                shiftcnt = 8 - pos;
                if((x.mode >> shiftcnt & 1) == 1)
                    modestr[pos + 1] = rwx[j];
            }
        }
        switch(x.type)
        {
            case TAR_DIR:
                modestr[0] = 'd';
                break;
            case TAR_BLK:
                modestr[0] = 'b';
                break;
            case TAR_CHR:
                modestr[0] = 'c';
                break;
        }
        printf("%s %lu %lu %s\n", modestr, x.size, x.mtime, x.name);
    }
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
int insert_entry(FILE *fh, const char *fname)
{
    struct tar_header head;
    struct stat fdat;
    size_t len;
    char cbuf[TAR_HEADER_SIZE];
    FILE *fin;
    memset(&head, 0, sizeof head);
    strcpy(head.name, fname);
    strcpy(head.ver, "00");
    stat(head.name, &fdat);
    head.size = fdat.st_size;
    head.mtime = fdat.st_mtime;
    head.uid = fdat.st_uid;
    head.gid = fdat.st_gid;
    head.mode = fdat.st_mode;
    head.devmajor = major(fdat.st_dev);
    head.devminor = minor(fdat.st_dev);
    if(S_ISREG(fdat.st_mode))
    {
        head.type = TAR_REG;
        fin = fopen(head.name, "r");
    }
    else if(S_ISDIR(fdat.st_mode))
    {
        head.type = TAR_DIR;
        head.size = 0;
        len = strlen(head.name);
        if(head.name[len - 1] != '/')
            head.name[len] = '/';
    }
    tar_write(fh, &head);
    if(head.type == TAR_REG)
    {
        if(fin == NULL)
        {
            fprintf(stderr, "Opening file %s", head.name);
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
    return head.type == TAR_DIR;
}
void insert_iterator_callback(void *arg, const char *dname)
{
    insert_entry((FILE*)arg, dname);
}
void append_entry_normal(FILE *fh, const char *name)
{
    if(insert_entry(fh, name))
        recursive_directory_iterator(name, insert_iterator_callback, (void*)fh);
}
void append_entry_concat(FILE *fh, const char *name)
{
    FILE *other = fopen(name, "r");
    if(other == NULL)
    {
        fprintf(stderr, "Reading %s", name);
        perror(" failed");
    }
    else
    {
        union tar_header_data rhead;
        int succ = tar_read_raw(other, &rhead);
        while(succ == 0)
        {
            fwrite(rhead.raw, 1, TAR_HEADER_SIZE, fh);
            succ = tar_read_raw(other, &rhead);
        }
        fclose(other);
    }
}
void append_arch(const char *fname, char *entries[], enum operation op)
{
    FILE *fh = fopen(fname, "r+");
    if(fh == NULL)
        perror("Archive file could not be opened for writing");
    else
    {
        void(*append_func)(FILE *fh, const char *name);
        tar_enumerate_headers(fh, append_arch_enum_callback, NULL);
        fseek(fh, -TAR_HEADER_SIZE, SEEK_CUR);
        switch(op)
        {
            case APPEND:
                append_func = append_entry_normal;
                break;
            case CONCAT:
                append_func = append_entry_concat;
                break;
            default:
                append_func = NULL;
        }
        for(char **it = entries; *it != NULL; ++it)
            append_func(fh, *it);
        tar_end_archive(fh);
        fclose(fh);
    }
}
void list_arch(const char *fname, char verbose)
{
    FILE *fh = fopen(fname, "r");
    if(fh == NULL)
        perror("Archive file could not be opened for reading");
    else
    {
        tar_enumerate_headers(fh, list_arch_enum_callback, verbose ? fh : NULL);
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
        for(char **it = entries; *it != NULL; ++it)
        {
            if(insert_entry(fh, *it))
                recursive_directory_iterator(*it, insert_iterator_callback, (void*)fh);
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
        char *arg, *target = NULL, nxtfile = 0, verbose = 0;
        for(int i = 1; optend == 1 && i < argl; ++i)
        {
            arg = argv[i];
            if(arg[0] == '-')
            {
                for(char *it = arg + 1; *it != '\0'; ++it)
                {
                    switch(*it)
                    {
                        case'A':
                            op = CONCAT;
                            break;
                        case'c':
                            op = CREATE;
                            break;
                        case'f':
                            nxtfile = 1;
                            break;
                        case'r':
                            op = APPEND;
                            break;
                        case't':
                            op = LIST;
                            break;
                        case'v':
                            verbose = *it;
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
            case APPEND:
            case CONCAT:
                append_arch(target, argv + optend, op);
                break;
            case CREATE:
                create_arch(target, argv + optend);
                break;
            case EXTRACT:
                extract_arch(target);
                break;
            case LIST:
                list_arch(target, verbose);
                break;
        }
    }
    return succ;
}
