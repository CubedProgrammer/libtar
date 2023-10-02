#include<stdlib.h>
#include<string.h>
#include"tar.h"
enum operation
{   LIST, CREATE, EXTRACT   };
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
                break;
            case EXTRACT:
                break;
            case LIST:
                break;
        }
    }
    return succ;
}
