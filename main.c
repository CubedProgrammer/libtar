#include<dirent.h>
#include<errno.h>
#include<fcntl.h>
#include<grp.h>
#include<pwd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/sysmacros.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include<utime.h>
#include"tar.h"
#include"str_int_map.h"
#define MAJOR 1
#define MINOR 0
#define PATCH 0
#define EX_KEEP 1
#define EX_SKIP 2
#define EX_UPDATE 3
#define EX_PERM 4
#define EX_TOUCH 010
#define EX_UNLNK 020
#define EX_VERBOSE 040
#define CR_VERIFY 0100
struct extract_options
{
    FILE *fh;
    short op;
};
const char dl_loader[]__attribute__((section(".interp")))="/usr/lib/ld-linux-x86-64.so.2";
struct tar_str_int_map tar_entry_date_map;
enum operation
{   APPEND, UPDATE, CONCAT, LIST, CREATE, EXTRACT   };
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
    enum operation op = *(enum operation*)arg;
    if(op == UPDATE)
    {
        struct tar_header x;
        tar_rtoh(&x, header);
        tar_simap_insert(&tar_entry_date_map, x.name, x.mtime);
    }
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
            case TAR_PIP:
                modestr[0] = 'p';
                break;
            case TAR_DIR:
                modestr[0] = 'd';
                break;
            case TAR_BLK:
                modestr[0] = 'b';
                break;
            case TAR_CHR:
                modestr[0] = 'c';
                break;
            case TAR_SYM:
                modestr[0] = 'l';
                break;
        }
        printf("%s %s/%s %9lu %lu %s", modestr, x.user, x.group, x.size, x.mtime, x.name);
        if(x.type == TAR_SYM)
            printf(" -> %s\n", x.lnk);
        else
            putchar('\n');
    }
    return 0;
}
size_t extract_arch_enum_callback(void *arg, union tar_header_data *header)
{
    struct tar_header x;
    struct extract_options exop = *(struct extract_options*)arg;
    long cnt = 0;
    tar_rtoh(&x, header);
    if(tar_entry_date_map.cnt == 0 || tar_simap_fetch(&tar_entry_date_map, x.name) == 1)
    {
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
        else if(x.type == TAR_SYM)
        {
            if(symlink(x.lnk, x.name))
            {
                fprintf(stderr, "symlink %s to %s", x.name, x.lnk);
                perror(" failed");
            }
        }
        else if(x.type == TAR_PIP)
        {
            if(mkfifo(x.name, x.mode))
            {
                fprintf(stderr, "mkfifo %s", x.name);
                perror(" failed");
            }
        }
        else
        {
            FILE *fout;
            char okay = 0, recent = 1;
            struct stat fdat;
            struct utimbuf ut;
            short owop = exop.op & 0b11;
            switch(owop)
            {
                case 3:
                    if(stat(x.name, &fdat) == 0)
                        recent = fdat.st_mtime > x.mtime;
                    else
                        recent = 0;
                case 2:
                case 1:
                    if(recent && access(x.name, F_OK) == 0)
                    {
                        if(exop.op == 1)
                            fprintf(stderr, "File %s already exists.\n", x.name);
                    }
                    else
                    {
                case 0:
                        if(exop.op & EX_UNLNK)
                            unlink(x.name);
                        fout = fopen(x.name, "wb");
                        if(fout == NULL)
                        {
                            fprintf(stderr, "Creating file %s", x.name);
                            perror(" failed");
                        }
                        else
                            okay = 1;
                    }
                    break;
            }
            if(okay)
            {
                long bytes, diff = 0;
                char cbuf[TAR_HEADER_SIZE];
                FILE *fh = exop.fh;
                for(; cnt < x.size; cnt += bytes)
                {
                    bytes = fread(cbuf, 1, TAR_HEADER_SIZE, fh);
                    if(bytes + cnt > x.size)
                        diff = cnt + bytes - x.size;
                    fwrite(cbuf, 1, bytes - diff, fout);
                }
                fclose(fout);
                if((exop.op & EX_TOUCH) == 0)
                {
                    ut.actime = x.mtime;
                    ut.modtime = x.mtime;
                    utime(x.name, &ut);
                }
                if(exop.op & 0b100)
                    chmod(x.name, x.mode);
            }
        }
        if(exop.op & EX_VERBOSE)
            printf("%s\n", x.name);
    }
    return cnt;
}
int insert_entry(FILE *fh, const char *fname, short op)
{
    struct tar_header head;
    union tar_header_data dat;
    struct stat fdat;
    size_t len;
    char cbuf[TAR_HEADER_SIZE];
    struct passwd *pdat;
    struct group *gdat;
    char ins = 1;
    FILE *fin;
    memset(&head, 0, sizeof head);
    strcpy(head.name, fname);
    strcpy(head.ver, "00");
    lstat(head.name, &fdat);
    head.size = 0;
    head.mtime = fdat.st_mtime;
    head.uid = fdat.st_uid;
    head.gid = fdat.st_gid;
    head.mode = fdat.st_mode;
    pdat = getpwuid(head.uid);
    gdat = getgrgid(head.gid);
    if(pdat != NULL)
        strncpy(head.user, pdat->pw_name, sizeof(head.user));
    if(gdat != NULL)
        strncpy(head.group, gdat->gr_name, sizeof(head.group));
    head.devmajor = major(fdat.st_dev);
    head.devminor = minor(fdat.st_dev);
    if(S_ISREG(fdat.st_mode))
    {
        head.type = TAR_REG;
        head.size = fdat.st_size;
        fin = fopen(head.name, "r");
    }
    else if(S_ISDIR(fdat.st_mode))
    {
        head.type = TAR_DIR;
        len = strlen(head.name);
        if(head.name[len - 1] != '/')
            head.name[len] = '/';
    }
    else if(S_ISCHR(fdat.st_mode))
        head.type = TAR_CHR;
    else if(S_ISBLK(fdat.st_mode))
        head.type = TAR_BLK;
    else if(S_ISFIFO(fdat.st_mode))
        head.type = TAR_PIP;
    else if(S_ISLNK(fdat.st_mode))
    {
        head.type = TAR_SYM;
        readlink(head.name, head.lnk, sizeof(head.lnk) - 1);
    }
    else if(S_ISSOCK(fdat.st_mode))
    {
        fprintf(stderr, "TAR cannot accept sockets, %s will be ignored.\n", head.name);
        ins = 0;
    }
    if(ins)
    {
        tar_htor(&dat, &head);
        tar_write_raw(fh, &dat);
        if(op & EX_VERBOSE)
        {
            printf("Inserted %s into the archive.\n", head.name);
            if(op & CR_VERIFY)
            {
                if(tar_verify(&dat))
                    printf("%s verified\n", head.name);
                else
                    printf("%s is invalid\n", head.name);
            }
        }
    }
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
    struct extract_options *op = arg;
    insert_entry(op->fh, dname, op->op);
}
void append_entry_normal(FILE *fh, const char *name, short op)
{
    struct extract_options crop;
    crop.fh = fh;
    crop.op = op;
    if(insert_entry(fh, name, op))
        recursive_directory_iterator(name, insert_iterator_callback, &crop);
}
void append_entry_concat(FILE *fh, const char *name, short op)
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
        unsigned verify = 0;
        int succ = tar_read_raw(other, &rhead);
        while(succ == 0)
        {
            if(verify == 0)
            {
                if(tar_verify(&rhead))
                {
                    for(unsigned i = 0; i < sizeof(rhead.header.size) - 1; ++i)
                        verify = verify << 3 | rhead.header.size[i] - '0';
                    verify = (verify + TAR_HEADER_SIZE - 1) / TAR_HEADER_SIZE;
                    fwrite(rhead.raw, 1, TAR_HEADER_SIZE, fh);
                }
                else
                    fprintf(stderr, "Invalid entry found in %s, skipping.\n", name);
            }
            else
            {
                --verify;
                fwrite(rhead.raw, 1, TAR_HEADER_SIZE, fh);
            }
            succ = tar_read_raw(other, &rhead) * !verify;
        }
        fclose(other);
    }
}
void append_entry_update(FILE *fh, const char *name, short op)
{
    struct stat fdat;
    stat(name, &fdat);
    if(fdat.st_mtime > tar_simap_fetch(&tar_entry_date_map, name))
        append_entry_normal(fh, name, op);
}
void append_arch(FILE *fh, char *entries[], enum operation op, short crop)
{
    void(*append_func)(FILE *fh, const char *name, short op);
    tar_enumerate_headers(fh, append_arch_enum_callback, &op);
    fseek(fh, -TAR_HEADER_SIZE, SEEK_CUR);
    switch(op)
    {
        case APPEND:
            append_func = append_entry_normal;
            break;
        case CONCAT:
            append_func = append_entry_concat;
            break;
        case UPDATE:
            append_func = append_entry_update;
            break;
        default:
            append_func = NULL;
    }
    for(char **it = entries; *it != NULL; ++it)
        append_func(fh, *it, crop);
    tar_end_archive(fh);
}
void list_arch(FILE *fh, char verbose)
{
    tar_enumerate_headers(fh, list_arch_enum_callback, verbose ? fh : NULL);
}
void create_arch(FILE *fh, char *entries[], short op)
{
    struct extract_options crop;
    crop.fh = fh;
    crop.op = op;
    for(char **it = entries; *it != NULL; ++it)
    {
        if(insert_entry(fh, *it, op))
            recursive_directory_iterator(*it, insert_iterator_callback, &crop);
    }
    tar_end_archive(fh);
}
void extract_arch(FILE *fh, char *entries[], short op)
{
    struct extract_options exop;
    for(char **it = entries; *it != NULL; ++it)
        tar_simap_insert(&tar_entry_date_map, *it, 1);
    exop.fh = fh;
    exop.op = op;
    tar_enumerate_headers(fh, extract_arch_enum_callback, (void*)&exop);
}
FILE *sendto(char *prog, char *target, int *pidp)
{
    int fds[2];
    int pid;
    FILE *handle = NULL;
    if(pipe(fds))
        perror("pipe failed");
    else
    {
        int ipipe = fds[0], opipe = fds[1];
        pid = fork();
        if(pid > 0)
        {
            close(ipipe);
            *pidp = pid;
            handle = fdopen(opipe, "w");
        }
        else if(pid < 0)
            perror("fork failed");
        else
        {
            int filedes;
            char *args[] = {prog, NULL};
            close(opipe);
            filedes = open(target, O_WRONLY | O_TRUNC | O_CREAT, 0644);
            dup2(ipipe, STDIN_FILENO);
            dup2(filedes, STDOUT_FILENO);
            close(ipipe);
            close(filedes);
            if(execvp(prog, args))
            {
                perror("execvp failed");
                exit(1);
            }
        }
    }
    return handle;
}
FILE *recvfrom(char *prog, char *target, int *pidp)
{
    int fds[2];
    int pid;
    FILE *handle = NULL;
    if(pipe(fds))
        perror("pipe failed");
    else
    {
        int ipipe = fds[0], opipe = fds[1];
        pid = fork();
        if(pid > 0)
        {
            close(opipe);
            *pidp = pid;
            handle = fdopen(ipipe, "r");
        }
        else if(pid < 0)
            perror("fork failed");
        else
        {
            char tostdout[] = "-c";
            char *args[] = {prog, tostdout, target, NULL};
            close(ipipe);
            dup2(opipe, STDOUT_FILENO);
            close(opipe);
            if(execvp(prog, args))
            {
                perror("execvp failed");
                exit(1);
            }
        }
    }
    return handle;
}
int main(int argl, char *argv[])
{
    int succ = 0;
    if(argl == 1)
    {
        printf("%s version %d.%d", *argv, MAJOR, MINOR);
        if(PATCH)
            printf(".%d", PATCH);
        puts(" https://github.com/CubedProgrammer/libtar");
        puts("Documentation at https://man7.org/linux/man-pages/man1/tar.1.html");
        puts("If the archive file is not specified, the standard output is used.");
        puts("Or the standard input for extraction.");
        puts("Compression method must be specified both when reading and writing.");
        printf("Example: `%s -czf arch.tar` to create and `%s -xzf arch.tar` to extract.\n", argv[0], argv[0]);
    }
    else
    {
        int optend = argl;
        enum operation op;
        FILE *fh;
        char *arg, *target = NULL;
        char *dirtarget = NULL;
        char openmode[3] = "r";
        char nxtfile = 0, nxtdir = 0;
        char verbose = 0, compress = 0;
        char longopt;
        short exop = 0;
        char gzipstr[] = "gzip";
        char gunzipstr[] = "gunzip";
        char *deflater[] = {gzipstr};
        char *inflater[] = {gunzipstr};
        char *extprog = NULL;
        int compresspid = 0;
        tar_simap_init(&tar_entry_date_map);
        for(int i = 1; i < optend; ++i)
        {
            arg = argv[i];
            longopt = 0;
            if(arg[0] == '-')
            {
                for(char *it = arg + 1; !longopt && *it != '\0'; ++it)
                {
                    switch(*it)
                    {
                        case'-':
                            longopt = 1;
                            ++it;
                        if(strcmp(it, "concatenate") == 0 || strcmp(it, "catenate") == 0)
                        {
                        case'A':
                            op = CONCAT;
                            strcpy(openmode + 1, "+");
                            break;
                        }
                        else if(strcmp(it, "directory") == 0)
                        {
                        case'C':
                            nxtdir = 1;
                            break;
                        }
                        else if(strncmp(it, "directory=", 10) == 0)
                            chdir(it + 10);
                        else if(strcmp(it, "create") == 0)
                        {
                        case'c':
                            op = CREATE;
                            openmode[0] = 'w';
                            break;
                        }
                        else if(strcmp(it, "file") == 0)
                        {
                        case'f':
                            nxtfile = 1;
                            break;
                        }
                        else if(strncmp(it, "file=", 5) == 0)
                        {
                            target = it + 5;
                            break;
                        }
                        else if(strcmp(it, "keep-old-files") == 0)
                        {
                        case'k':
                            exop |= 1;
                            break;
                        }
                        else if(strcmp(it, "touch") == 0)
                        {
                        case'm':
                            exop |= EX_TOUCH;
                            break;
                        }
                        else if(strcmp(it, "perserve-permissions") == 0 || strcmp(it, "same-permissions") == 0)
                        {
                        case'p':
                            exop |= EX_PERM;
                            break;
                        }
                        else if(strcmp(it, "append") == 0)
                        {
                        case'r':
                            op = APPEND;
                            strcpy(openmode + 1, "+");
                            break;
                        }
                        else if(strcmp(it, "list") == 0)
                        {
                        case't':
                            op = LIST;
                            break;
                        }
                        else if(strcmp(it, "unlink-first") == 0)
                        {
                        case'U':
                            exop |= EX_UNLNK;
                            break;
                        }
                        else if(strcmp(it, "update") == 0)
                        {
                        case'u':
                            op = UPDATE;
                            strcpy(openmode + 1, "+");
                            break;
                        }
                        else if(strcmp(it, "verbose") == 0)
                        {
                        case'v':
                            verbose = *it;
                            exop |= EX_VERBOSE;
                            break;
                        }
                        else if(strcmp(it, "verify") == 0)
                        {
                        case'W':
                            exop |= CR_VERIFY;
                            break;
                        }
                        else if(strcmp(it, "extract") == 0 || strcmp(it, "get") == 0)
                        {
                        case'x':
                            op = EXTRACT;
                            break;
                        }
                        else if(strcmp(it, "gzip") == 0)
                        {
                        case'z':
                            compress = 1;
                            break;
                        }
                        else if(strcmp(it, "skip-old-files") == 0)
                        {
                            exop |= 2;
                            break;
                        }
                        else if(strcmp(it, "keep-newer-files") == 0)
                        {
                            exop |= 3;
                            break;
                        }
                        else if(longopt)
                            fprintf(stderr, "Unrecognized option %s will be ignored.\n", it);
                        else
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
            else if(nxtdir)
            {
                dirtarget = arg;
                nxtdir = 0;
            }
            else
                optend = i;
        }
        if(target == NULL)
            fh = op == EXTRACT || op == LIST ? stdin : stdout;
        else if(compress)
        {
            if(op == CREATE)
                fh = sendto(extprog = deflater[compress - 1], target, &compresspid);
            else
                fh = recvfrom(extprog = inflater[compress - 1], target, &compresspid);
        }
        else
            fh = fopen(target, openmode);
        if(dirtarget != NULL)
            chdir(dirtarget);
        if(fh == NULL)
        {
            fprintf(stderr, "Opening %s", target);
            perror(" failed");
        }
        else
        {
            switch(op)
            {
                case APPEND:
                case CONCAT:
                case UPDATE:
                    append_arch(fh, argv + optend, op, exop);
                    break;
                case CREATE:
                    create_arch(fh, argv + optend, exop);
                    break;
                case EXTRACT:
                    extract_arch(fh, argv + optend, exop);
                    break;
                case LIST:
                    list_arch(fh, verbose);
                    break;
            }
            fclose(fh);
            if(compresspid != 0)
            {
                int status;
                waitpid(compresspid, &status, 0);
                status = WEXITSTATUS(status);
                if(status != 0)
                    fprintf(stderr, "%s exited with status %d\n", extprog, status);
            }
        }
        tar_simap_free(&tar_entry_date_map);
    }
    return succ;
}
