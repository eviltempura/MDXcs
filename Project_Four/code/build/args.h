#ifndef _ARGS_H_
#define _ARGS_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

typedef struct Room Room;
struct Room {
    Room* next;
    int64_t roomId;
};

typedef struct Person Person;
struct Person {
    char name[1024];
    int64_t enterTime;
    int64_t leaveTime;
    int64_t speedTime;
    int64_t roomId;
    int8_t isGallery;
    int8_t isGuest;
    Room* head;
    Room* tail;
};

typedef struct LogArgs LogArgs;

struct LogArgs {
    char name[1024];
    int64_t time;
    int8_t isGuest;
    int8_t isEnter;
    int8_t isLeave;
    int64_t roomID;
};

int person_count = 0;
int person_capacity = 0;
int64_t lastTime = 0;
Person** people;

void free_people() {
    int i = 0;
    for (i = 0; i < person_count; ++i) {
        Room* cur = people[i]->head;
        while(cur != NULL) {
            Room* nx = cur->next;
            free(cur);
            cur = nx;
        }
        free(people[i]);
    }
    free(people);
}

int cmpare(const void* elem1, const void* elem2) {
    return (strcmp((char*)elem1, (char*)elem2));
}

int cmp_roomId(const void* p1, const void* p2) {
    const Person* pp1 = *(Person**)p1;
    const Person* pp2 = *(Person**)p2;
    if (pp1->roomId < pp2->roomId) {
        return -1;
    } if (pp1->roomId == pp2->roomId) {
        if(strcmp(pp1->name,pp2->name)<0){
		return -1;
	}else{
	    return 1;
	}
	
    }
    return 1;
}

int cmp_name(const void* p1, const void* p2) {
    const Person* pp1 = *(Person**)p1;
    const Person* pp2 = *(Person**)p2;
    if(strcmp(pp1->name,pp2->name)<0){
	return -1;
    }else{
	return 1;
    }
}


void handler_s_name(int8_t isGuest) {
    int i = 0;
    int cnt = 0;
    for (i = 0; i < person_count; ++i) {
        if (people[i]->isGallery == 1 && people[i]->isGuest == isGuest) {
            cnt += 1;
        }
    }
    int j = 0;
    for (i = 0; i < person_count; ++i) {
        if (people[i]->isGallery == 1 && people[i]->isGuest == isGuest) {
            printf("%s", people[i]->name);
            j++;
            if (j != cnt) {
                printf(",");
            }
        }
    }
    printf("\n");
}


void handler_s_room() {
    int i = 0;
    int64_t pre_id = -1;
    for (i = 0; i < person_count; ++i) {
        if (people[i]->isGallery == 0 || people[i]->roomId == -1) {
            continue;
        }
        if (pre_id != people[i]->roomId) {
            if (pre_id != -1) {
                printf("\n");
            }
            printf("%ld: ", people[i]->roomId);
        } else {
            printf(",");
        }
        printf("%s", people[i]->name);
        pre_id = people[i]->roomId;
    }
    
}


void handler_s() {
    qsort(people, person_count, sizeof(Person*), cmp_name);
    handler_s_name(0);
    handler_s_name(1);
    qsort(people, person_count, sizeof(Person*), cmp_roomId);
    handler_s_room();
}


void handler_r(int8_t isGuest, const char* name) {
    int i = 0;
    for (i = 0; i < person_count; i++) {
        if (strcmp(people[i]->name, name) == 0 && isGuest == people[i]->isGuest) {
            Room* cur = people[i]->head;
            int8_t flag = 0;
            while(cur != NULL) {
                printf("%ld", cur->roomId);
                if (cur->next != NULL) {
                    printf(",");
                }
                cur = cur->next;
                flag = 1;
            }
            if (people[i]->roomId != -1) {
                if (flag == 1) {
                    printf(",");
                }
                printf("%ld", people[i]->roomId);
            }
            printf("\n");
            break;
        }
    }
}


void handler_t(int8_t isGuest, const char* name) {
    int i = 0;
    for (i = 0; i < person_count; i++) {
        if (strcmp(people[i]->name, name) == 0 && isGuest == people[i]->isGuest) {
           
            if (people[i]->isGallery == 1) {
                printf("%ld\n", people[i]->speedTime + (lastTime - people[i]->enterTime));
            } else {
                printf("%ld\n", people[i]->speedTime);
            }
            break;
        }
    }
}

Room* new_room(int64_t roomId) {
    Room* room = (Room*)malloc(sizeof(Room));
    room->next = NULL;
    room->roomId = roomId;
    return room;
}

void add_room(Room** head, Room** tail, int64_t roomId) {
    if (*head == NULL) {
        (*tail) = (*head) = new_room(roomId);
    } else {
        Room* room = new_room(roomId);
        (*tail)->next = room;
        (*tail) = room;
    }
}

int set_person(Person* p, LogArgs* arg) {
    if(arg->isEnter == 1 && arg->roomID == -1 && p->isGallery == 0) { // enter gallery
        p->enterTime = arg->time;
        p->isGallery = 1;
        p->isGuest = arg->isGuest;
        return 0;
    }
    if (arg->isLeave == 1 && arg->roomID == -1 && p->isGallery == 1 && p->roomId == -1) { // levae gallery
        p->leaveTime = arg->time;
        p->speedTime += p->leaveTime - p->enterTime;
        p->isGallery = 0;
        p->isGuest = arg->isGuest;
        return 0;
    }

    if (arg->isEnter == 1 && arg->roomID != -1 && p->isGallery == 1 && p->roomId == -1) { // enter room
        p->roomId = arg->roomID;
        p->isGuest = arg->isGuest;
        return 0;
    }
    if (arg->isLeave == 1 && arg->roomID != -1 && p->isGallery == 1 && p->roomId != -1) { // leave room
        add_room(&(p->head), &(p->tail), p->roomId);
        p->roomId = -1;
        p->isGuest = arg->isGuest;
        return 0;
    }
    return -1;
}

Person* new_person(const char* name) {
    Person* p = (Person*) malloc(sizeof(Person));
    p->roomId = -1;
    p->enterTime = 0;
    p->leaveTime = 0;
    p->speedTime = 0;
    p->isGallery = 0;
    p->isGuest = 0;
    p->head = NULL;
    p->tail = NULL;
    strcpy(p->name, name);
    return p;
}


int update_person(LogArgs* arg) {
    int i = 0;
    int res = -1;
    for(i = 0; i < person_count; i++) {
        if (strcmp(arg->name, people[i]->name) == 0 && arg->isGuest == people[i]->isGuest) {
            res = set_person(people[i], arg);
            break;
        }
    }

    if (i == person_count) {
        if (person_capacity <= person_count) {
            people = (Person**)realloc(people, (person_capacity + 1) * sizeof(Person*));
            people[i] = new_person(arg->name);
            person_capacity += 1;
        }
        res = set_person(people[i], arg);
        if (res == 0) {
            person_count += 1;
        }
    }
    if (res == 0) {
        lastTime = arg->time;
    }
    return res;
}

LogArgs opt_parser(int32_t argc, char** argv) {
	int32_t c;

    LogArgs arg;
    arg.isGuest = 0;
    arg.roomID = -1;
    arg.isEnter = 0;
    arg.isLeave = 0;
    arg.time = 0;
    optind = 1;

	while ((c = getopt(argc, argv, "R:T:AILE:G:")) != -1) {
		switch (c) {
		case 'R':
            arg.roomID = atol(optarg);
			break;
		case 'T':
            arg.time = atol(optarg);
			break;
		case 'E':
			strcpy(arg.name, optarg);
			break;
		case 'G':
			strcpy(arg.name, optarg);
            arg.isGuest = 1;
			break;
		case 'L':
            arg.isLeave = 1;
			break;
		case 'A':
            arg.isEnter = 1;
			break;
		}
	}

	return arg;
}

int split_line(char*** lines) {
    // char* line = strtok(result, "\n");
    char *line = strtok(NULL, "\n");
    int count = 0;
    while (line != NULL) {
        (*lines) = (char**)realloc(*lines, (count + 1) * sizeof(char*));
        (*lines)[count] = (char*)malloc(strlen(line)+1);
        strcpy((*lines)[count], line);
        count += 1;
        line = strtok(NULL, "\n");
    }
    return count;
}

void process_line(char* line) {
    int32_t argc = 1;
    char** argv = NULL;
    char* r = strtok(line, " ");
    while (r != NULL) {
        argv = (char**) realloc(argv, (argc + 1) * sizeof(char*));
        argv[argc] = (char*) malloc(strlen(r)+1);
        strcpy(argv[argc], r);
        r = strtok(NULL, " ");
        argc += 1;
    }
    argv[0] = (char*) malloc(3);
    strcpy(argv[0], "la");
    LogArgs arg = opt_parser(argc, argv);
    for(int i = 0; i < argc; i++) {
        free(argv[i]);
    }
    free(argv);
    update_person(&arg);
}


char* tohex(const unsigned char* content) {
    static char buf[128] = {0};
    size_t i = 0;
    for (i = 0; i < 32; ++i) {
        sprintf(buf+ i * 2, "%02x", content[i]);
    }
    return buf;
}

unsigned char* sha256(const char* content) {
    EVP_MD_CTX *evpCtx;
    evpCtx = EVP_MD_CTX_create();
    EVP_DigestInit_ex(evpCtx, EVP_sha256(), NULL);
    unsigned int len = strlen(content);
    EVP_DigestUpdate(evpCtx, content, len);
    unsigned char* result = (unsigned char*)malloc(32);
    EVP_DigestFinal_ex(evpCtx, result, &len);
    EVP_MD_CTX_destroy(evpCtx);
    return result;
}

unsigned char* md5hash(const char* key) {
    EVP_MD_CTX *evpCtx;
    evpCtx = EVP_MD_CTX_create();
    EVP_DigestInit_ex(evpCtx, EVP_md5(), NULL);
    unsigned int len = strlen(key);
    EVP_DigestUpdate(evpCtx, key, len);
    unsigned char* result = (unsigned char*)malloc(32);
    EVP_DigestFinal_ex(evpCtx, result, &len);
    EVP_MD_CTX_destroy(evpCtx);
    return result;
}


char* decrypy(const char* filename, const char* key) {
    unsigned char* token = md5hash(key);
    unsigned char iv[16] = {0};
    FILE* f = fopen(filename, "ab+");
    if (f == NULL) {
        return NULL;
    }
    unsigned char in[1024], out[1024 + EVP_MAX_BLOCK_LENGTH];

    EVP_CIPHER_CTX ctx;
    EVP_CIPHER_CTX_init(&ctx);
    EVP_CipherInit_ex(&ctx, EVP_aes_128_cbc(), NULL, token, iv, 0);
    int suminlen = 0;
    char *result = NULL;
    int outlen;
    while (1) {
        int inlen = fread(in, 1, 1024, f);
        if (inlen <= 0) {
            break;
        }
        if (!EVP_CipherUpdate(&ctx, out, &outlen, in, inlen)) {
            EVP_CIPHER_CTX_cleanup(&ctx);
            return NULL;
        }
        result = (char*)realloc(result, suminlen + outlen);
        strncpy(result + suminlen, out, outlen);
        suminlen += outlen;
    }

    if(!EVP_CipherFinal_ex(&ctx, out, &outlen)) {
        EVP_CIPHER_CTX_cleanup(&ctx);
        return NULL;
    }
    result = (char*)realloc(result, suminlen + outlen);
    strncpy(result + suminlen, out, outlen);
    EVP_CIPHER_CTX_cleanup(&ctx);
    free(token);
    fclose(f);
    return result;
}






#endif /* ARGS_H_ */
