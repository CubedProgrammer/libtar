#include<stdlib.h>
#include<string.h>
#include"tar.h"
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
}
void extract_arch(const char *fname)
{
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
