#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include "loghelper.h"

int parse_cmdline(int argc, char* argv[], char batch);

char checkTimeValid(int time, char* auditLog, int* eventBounds, int numEvents) {
	if(numEvents == 0) {
	   	return true;
	}
	int lastTime, room;
	char current, guest, arriving;
	char* name;
	parseEvent(auditLog, eventBounds[numEvents-1], eventBounds[numEvents], &lastTime, &current, &guest, &name, &arriving, &room);
	if(lastTime >= time) {
		return false;
   	}
	free(name);
	return true;
}

char checkActionValid(char targetArrive, int targetRoom, char* targetName, char targetGuest, char* auditLog, int* eventBounds, int numEvents) {
	int time, room;
	char current, guest, arriving;
	char* name;
	char hasRecord = false;
	int index = getMostRecentEventIndex(auditLog, eventBounds, numEvents, targetName, targetGuest);
	if(index != -1) {
		parseEvent(auditLog, eventBounds[index], eventBounds[index+1], &time, &current, &guest, &name, &arriving, &room);
		free(name);
		//Trying to enter 
		if(targetArrive) {
			//Still in other room
			if(arriving && room != -1) {
				return false;
			}
			//Not in gallery, but trying to enter room
			if(!arriving && room == -1 && targetRoom != -1) {
				return false;
			}
		}
		//Trying to leave
		else {
			//Trying to leave wrong room
			if(arriving && room != targetRoom) {
				return false;
			}
			//Not in gallery
			if(!arriving && room == -1) {
				return false;
			}
		}
	}
	//First occurance for person
	else {
		return (targetArrive && targetRoom == -1);
	}
	return true;
}

void markOld(char* auditLog, int i, int* eventBounds) {
	auditLog[eventBounds[i] + 10 + 1] = 'O';
}

void processBatch(char* batchFile) {
	FILE *batchInput = fopen(batchFile, "r");

	if(!batchInput) {
		error("No batch file supplied");
	}
	char* line = NULL;
	//Malloced buffer size used by getline
	size_t len = 0;
	//Actual size of current string
	ssize_t read;

	while ((read = getline(&line, &len, batchInput)) != -1) {
		//count spaces/num args in string
		//printf("[%s]", line);
		int numArgs = 0;
		char prevSpace = true;
		for(int i = 0; i < read; i++) {
			if(line[i] == ' ') {
				prevSpace = true;
			}else {
				if(prevSpace) {
					numArgs++;
				}
				prevSpace = false;
			}
		}
		numArgs++;
		char* args[numArgs];

		char *arg = strtok(line, " ");
		int i = 1;
		while( arg != NULL ) {
			if(i > numArgs) {
				//error("Invalid batch file");
			}
			args[i] = arg;
			if(args[i][strlen(arg)-1] == '\n') {
				args[i][strlen(arg)-1] = '\0';
			}
			arg = strtok(NULL, " ");
			i++;
		}
		
		optind = 0;
		parse_cmdline(numArgs, args, true);
	}

	fclose(batchInput);
	free(line);
	exit(0);
}
int parse_cmdline(int argc, char *argv[], char isBatch) {

	int opt = -1;
	int is_good = -1;

	long int longTime = -1;
	char* end = NULL;
  	int time = -1;

	long int longRoom = -1;
  	int room = -1;

  	char* name = NULL;
  	char arriving = -1;
  	char guest = -1;

  	char* key = NULL;

	char* logPath = NULL;
	char* batchPath = NULL;


	//printf("Command: ");
	//pick up the switches
	while ((opt = getopt(argc, argv, "T:K:E:G:ALR:B:")) != -1) {
		//printf("%c = %s ", opt, optarg);
		switch(opt) {
			case 'B':
				//batch file
				if(isBatch) {
					error("Batch in batch");
				}
				batchPath = optarg;
				break;

			case 'T':
				//Timestamp
				longTime = strtol(optarg, &end, 10);
				//Check that there are not trailing non-numbers
				if(optarg[0] == '\0' || end[0] != '\0') {
					error("time format");
				}
				//Check that time is in bounds
				if(longTime <= 0 || longTime > 1073741823) {
					error("time range");
				}
				time = (int)longTime;
				break;

			case 'K':
				//secret token
				key = optarg;
				if(!isAlphanumeric(key)) {
					error("token characters");
				}
				break;

			case 'A':
				//arrival
				arriving = true;
				break;

			case 'L':
				//departure
				arriving = false;
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

			case 'R':
				//room ID
				longRoom = strtol(optarg, &end, 10);
				//Check that there are not trailing non-numbers
				if(optarg[0] == '\0' || end[0] != '\0') {
					error("Room format");
				}
				//Check that room number is in bounds
				if(longRoom < 0 || longRoom > 1073741823) {
					error("Room range");
				}
				room = (int)longRoom;
				break;

			default:
				//unknown option, leave
				break;
		}

	}
	//printf("\n");


	//pick up the positional argument for log path
	if(optind < argc) {
		logPath = argv[optind];
	}

	//Batch
	if(batchPath && arriving == -1) {
		processBatch(batchPath);
	}
	//Check args
	if(key == NULL || name == NULL || time == -1 || guest == -1 || arriving == -1 || logPath == NULL) {
		error("Not enough args");
	}

	int* eventBounds = NULL;
	unsigned char* auditLog;
   	if(!decrypt_log(key, logPath, &auditLog)) {
		error("Integrity voilation");
	}
	int numEvents = split_message(auditLog, &eventBounds);

	//Event format: TIME(Always 10 digits)(Old/New) (G/E) NAME (A/L) [ROOM# -1 if N/A]
	//TIME(10)+space+(N/O)+space+(G/E)+space+NAME+space+(A/L)+space+ROOM(Always 10 Digits)+\n
	int length = 10+1+1+1+1+1+strlen(name)+1+1+1+10+1;
	char line[length];
	if(!checkTimeValid(time, auditLog, eventBounds, numEvents)) {
		error("Time invalid");
	}
	if(!checkActionValid(arriving, room, name, guest, auditLog, eventBounds, numEvents)) {
		error("Action invalid");
	}
	sprintf(line, "%010d N %s %s %s %010d\n", time, guest?"G":"E", name, arriving?"A":"L", room);
	int index = getMostRecentEventIndex(auditLog, eventBounds, numEvents, name, guest);
	if(index != -1){ 
		markOld(auditLog, index, eventBounds);
	}
	encrypt_write_log(key, logPath, auditLog, line);
	free(auditLog);

	return is_good;
}


int main(int argc, char *argv[]) {
	int result;
	result = parse_cmdline(argc, argv, false);

	return 0;
}