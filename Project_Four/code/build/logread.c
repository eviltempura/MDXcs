#include "args.h"


int main(int argc, char *argv[])
{

    char token[1024];
    int isPrint = 0;
    char name[1024];
    int isR = 0;
    int isGuest = 0;
    int isTime = 0;
    int isEmployee = 0;
    int c;
    optind = 1;
    while ((c = getopt(argc, argv, "K:E:G:ISTR")) != -1) {
        switch (c) {
        case 'K':
            strcpy(token, optarg);
            break;
        case 'S':
            isPrint = 1;
            break;
        case 'R':
            isR = 1;
            break;
        case 'E':
            strcpy(name, optarg);
            isEmployee = 1;
            break;
        case 'G':
            strcpy(name, optarg);
            isGuest = 1;
            break;
        case 'I':
            printf("unimplemented\n");
            return 0;
        case 'T':
            isTime = 1;
            break;
        }
    }

    /* printf("%d - %d\n", optind, argc); */
    /* printf("%d %d %d %d\n", isR, isPrint, is_employee, is_guest); */
    if (optind == argc || (isR + isPrint + isTime) != 1||(isPrint != 1&& isGuest + isEmployee != 1)) {
        printf("invalid\n");
        return 255;
    }

    /* printf("%s\n", argv[optind]); */
    char* result = decrypy(argv[optind], token);
    if (result == NULL) {
        printf("integrity violation\n");
        return 255;
    }
    char** lines = NULL;
    char* hash  = strtok(result, "\n");
    char* left = result + strlen(hash) + 1;
    unsigned char* my_hash = sha256(left);
    if (strcmp(tohex(my_hash), hash) != 0) {
        printf("integrity violation\n");
        return 255;
    }
    free(my_hash);
    int count = split_line(&lines);
    int i = 0;
    for (i = 0; i < count; i++) {
        process_line(lines[i]);
    }
    if (isPrint == 1) {
        // -S
        handler_s();
    } else if (isR == 1) {
        // -R
        handler_r(isGuest, name);
    } else if (isTime == 1) {
        // -T
        handler_t(isGuest, name);
    }
    free_people();
    free(result);
    for (i = 0; i < count; i++) {
        free(lines[i]);
    }
    free(lines);
    return 0;
}
