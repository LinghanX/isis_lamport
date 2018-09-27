#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (int argc, char **argv) {
    int option;
    int pflag = 0;
    int hflag = 0;
    int cflag = 0;

    while ((option = getopt(argc, argv, "p:h:c:")) != -1) {
        switch (option) {
            case 'p':
                printf("get p\n");
                printf("%s\n", optarg);
                pflag = 1;
                break;
            case 'h':
                printf("get h\n");
                printf("%s\n", optarg);
                hflag = 1;
                break;
            case 'c':
                printf("get c\n");
                printf("%s\n", optarg);
                cflag = 1;
                break;
            case '?':
                if (optopt == 'p')
                    fprintf(stderr, "Option -%c needs argument\n", optopt);
                else fprintf(stderr, "Unknown option -%c. \n", optopt);
                break;
            default:
                fprintf(stderr, "error");
        }
    }
    if (cflag == 0 || hflag == 0 || pflag == 0)
        fprintf(stderr, "please input all required options: c, h and p\n");

    return 0;
}