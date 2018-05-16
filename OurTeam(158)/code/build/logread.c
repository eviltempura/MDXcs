#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "loghelper.h"

//Finds the smallest room number larger than lowerBound
//Room must have at least one person in it
//Returns -1 if lowerBound is max
int findMinRoom(char* auditLog, int* eventBounds, int numEvents, int lowerBound) {
	int min = -1;
	int time, room;
	char current, guest, arriving;
	char* name;
	char found = false;
	for(int i = 0; i < numEvents; i++) {
		parseEvent(auditLog, eventBounds[i], eventBounds[i+1], &time, &current, &guest, &name, &arriving, &room);
		free(name);
		if(current && arriving && room != -1) {
			if((min==-1 || room < min) && (lowerBound==-1 || room > lowerBound)) {
				min = room;
				found = true;
			}
		}
	}
	if(!found) {
		return -1;
	}
	return min;

}

//Find the first name in lexographic order that comes after lowerBound.
//if targetGuest != -1, only correctGuest type is considered
//if targetRoom != -1, only correct room is considered
//Only current names are considered
//Returns NULL if lowerBound is max name
char* findMinName(char* auditLog, int* eventBounds, int numEvents, char* lowerBound, char targetGuest, int targetRoom) {
	char* min = NULL;
	int time, room;
	char current, guest, arriving;
	char* name;
	char found = false;
	for(int i = 0; i < numEvents; i++) {
		parseEvent(auditLog, eventBounds[i], eventBounds[i+1], &time, &current, &guest, &name, &arriving, &room);
		if(current && !(!arriving && room == -1) && (targetGuest == -1 || guest==targetGuest) && (targetRoom == -1 || (arriving && room == targetRoom))) {
			if((!min || strcmp(name, min) < 0) && (!lowerBound || strcmp(name, lowerBound)>0)) {
				if(min) {
					free(min);
				}
				min = name;
				found = true;
			}
		}
	}
	if(!found) {
		return NULL;
	}
	//Name is on the heap from parse
	return min;
}

void printRoom(char* auditLog, int* eventBounds, int numEvents, int room) {
	printf("%d: ", room);
	char* minName = NULL;
	char alreadyPrinted = false;
	//Print guests
	do {
		minName = findMinName(auditLog, eventBounds, numEvents, minName, -1, room);
		if(minName != NULL) {
			if(alreadyPrinted) {
				printf(",");
			}
			printf("%s", minName);
			alreadyPrinted = true;
		}
	} while(minName);
}

void printState(char* auditLog, int* eventBounds, int numEvents) {
	char* minName = NULL;
	char alreadyPrinted = false;
	//Print employees
	do {
		minName = findMinName(auditLog, eventBounds, numEvents, minName, false, -1);
		if(minName != NULL) {
			if(alreadyPrinted) {
				printf(",");
			}
			printf("%s", minName);
			alreadyPrinted = true;
		}
	} while(minName);
	printf("\n");

	minName = NULL;
	alreadyPrinted = false;
	//Print guests
	do {
		minName = findMinName(auditLog, eventBounds, numEvents, minName, true, -1);
		if(minName != NULL) {
			if(alreadyPrinted) {
				printf(",");
			}
			printf("%s", minName);
			alreadyPrinted = true;
		}
	} while(minName);
	printf("\n");

	//Print rooms
	int minRoom = -1;
	alreadyPrinted = false;
	do {
		minRoom = findMinRoom(auditLog, eventBounds, numEvents, minRoom);
		if(minRoom != -1) {
			//Print all people in this room in order
			if(alreadyPrinted) {
				printf("\n");
			}
			alreadyPrinted = true;
			printRoom(auditLog, eventBounds, numEvents, minRoom);
		}
	} while(minRoom != -1);
}

void printRooms(char* auditLog, int* eventBounds, int numEvents, char* targetName, char targetGuest) {
	int time, room;
	char current, guest, arriving;
	char* name;
	char alreadyPrinted = false;
	for(int i = 0; i < numEvents; i++) {
		parseEvent(auditLog, eventBounds[i], eventBounds[i+1], &time, &current, &guest, &name, &arriving, &room);
		if(!strcmp(name, targetName) && targetGuest == guest) {
			if(arriving) {
				if(room != -1){	if(alreadyPrinted) {
						printf(",");
					}
					printf("%d", room);
					alreadyPrinted = true;
				}
			}
		}
		free(name);
	}
	if(alreadyPrinted) {
		printf("\n");
	}

}

int main(int argc, char *argv[]) {
	int   opt,len;
	char  *logPath = NULL;

	//char time = false;
	char rooms = false;
	char state = false;
	char guest = -1;
	char employee = -1;
	char* name = NULL;
	char* key = NULL;

	while ((opt = getopt(argc, argv, "K:SRE:G:TI")) != -1) {
		switch(opt) {
			case 'T':
				printf("unimplemented");
				exit(0);
				break;

			case 'I':
				printf("unimplemented");
				exit(0);
				break;

			case 'S':
				if(rooms) {
					error("Two contradictory commands given");
				}
				state = true;
				break;

			case 'R':
				if(state) {
					error("Two contradictory commands given");
				}
				rooms = true;
				break;

			case 'K':
				//secret token
				key = optarg;
				if(!isAlphanumeric(key)) {
					error("token characters");
				}
				break;

			case 'E':
				//employee name
				guest = false;
				name = optarg;
				if(!isAlphabetical(name)) {
					error("Employee name");
				}
				break;

			case 'G':
				//guest name
				guest = true;
				name = optarg;
				if(!isAlphabetical(name)) {
					error("Guest name");
				}
				break;
		}
	}

	if(optind < argc) {
		logPath = argv[optind];
	}

	//check args
	if(state) {
		if(!key || !logPath) {
			error("Not enough args");
		}
	} else if(rooms) {
		if(!name || guest == -1 || !key || !logPath) {
			error("Not enough args");
		}
	} else{
		error("Not enough args");
	}

	int* eventBounds = NULL;
	unsigned char* auditLog;
   	if(!decrypt_log(key, logPath, &auditLog)) {
		printf("Integrity voilation");
		exit(255);
	}
	int numEvents = split_message(auditLog, &eventBounds);

	if(rooms) {
		printRooms(auditLog, eventBounds, numEvents, name, guest);
	} 
	if(state) {
		printState(auditLog, eventBounds, numEvents);
	}

}