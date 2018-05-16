#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/conf.h>
#include "loghelper.h"

void handleErrors() { ERR_print_errors_fp(stderr);
	abort();
}

void error(const char* msg) {
	#if DEBUG
	printf("%s\n", msg);
	#endif 
	printf("invalid\n");
	exit(255);
}

void parseEvent(char* audit, int start, int end, int* time, char* current, char* guest, char** name, char* arriving, int* room) {
	if(end - start <= FORMAT_LENGTH) {
		error("Event bounds");
	}
	char currentChar = 0;
	char guestChar = 0;
	char arriveChar = 0;
	*name = malloc(end - start - FORMAT_LENGTH + 1);
	if((*name) == NULL) {
		error("Could not allocate");
	}
	int numRead = sscanf(audit+start, "%d %s %s %s %s %d\n", time, &currentChar, &guestChar, *name, &arriveChar, room); if(numRead != 6) {
		error("Event format");
	}
	*current = (currentChar=='N')?true:false;
	*guest = (guestChar=='G')?true:false;
	*arriving = (arriveChar=='A')?true:false;
}

int split_message(char* str, int** returnarr) {
	int newlines = 0;
	int character;
	int len = strlen(str);
	int count;

	for(count = 0; count < len; count++) {
		character = str[count];
		if(character == '\n') {
			newlines++;
		}
	}

	*returnarr = calloc(newlines + 1, sizeof(int));
	if((*returnarr) == NULL) {
		error("Could not allocate");
	}
	(*returnarr)[0] = 0;
	int i = 1;

	for(count = 0; count < len; count++) {
		character = str[count];
		if(character == '\n') {
			(*returnarr)[i++] = count+1;
		}
	}

	return newlines;
}

char isAlphabetical(char* str) {
	int i = 0;
	while(str[i] != '\0') {
		if((str[i] < 'a' || str[i] > 'z') && (str[i] < 'A' || str[i] > 'Z')) {
			return false;
		}
		i++;
	}
	return true;
}

char isAlphanumeric(char* str) {
	int i = 0;
	while(str[i] != '\0') {
		if((str[i] < '0' || str[i] > '9')
			   	&& (str[i] < 'a' || str[i] > 'z')
			   	&& (str[i] < 'A' || str[i] > 'Z')) {
			return false;
		}
		i++;
	}
	return true;

}

//Returns the start index of eventBounds for the event
int getMostRecentEventIndex(char* auditLog, int* eventBounds, int numEvents, char* targetName, char targetGuest) {
	int time, room;
	char current, guest, arriving;
	char* name;
	char hasRecord = false;
	for(int i = 0; i < numEvents; i++) {
		parseEvent(auditLog, eventBounds[numEvents-i-1], eventBounds[numEvents-i], &time, &current, &guest, &name, &arriving, &room);
		//If log refers to same person and is current
		if(current && !strcmp(targetName, name) && targetGuest == guest) {
			return numEvents-i-1;
		}
	}
	return -1;
}

void hashKey(const unsigned char* token, unsigned char* hashedToken) {
	int md_len;
	EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
	EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
	EVP_DigestUpdate(mdctx, token, strlen(token));
	EVP_DigestFinal_ex(mdctx, hashedToken, &md_len);
	EVP_MD_CTX_cleanup(mdctx);
}

void encrypt_write_log(const unsigned char* shortToken, const char* file_name, const unsigned char* lines, const unsigned char *append) {
	//Hash token to 256 bits
	unsigned char token[32];
	hashKey(shortToken, token);

	EVP_CIPHER_CTX *context = EVP_CIPHER_CTX_new();

	//Length of the total message is length of both messages not including terminators. Just characters
    int msg_len = strlen(lines) + strlen(append);
    unsigned char ciphertext[msg_len+16];
    int len, cipher_len;
    unsigned const char iv[16] = {0};

	//Message with null terminator
    unsigned char msg[msg_len + 1];
    strcpy(msg, lines);
    strcat(msg, append);
	msg[msg_len] = '\0';


    EVP_EncryptInit_ex(context, EVP_aes_256_gcm(), NULL, token, iv);
    EVP_EncryptUpdate(context, ciphertext, &cipher_len, msg, msg_len+1);
    EVP_EncryptFinal_ex(context, ciphertext + cipher_len, &len);
    cipher_len += len;

    char tag[12];
    if(EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) {
        handleErrors();
    }

    EVP_CIPHER_CTX_free(context);

	/* open the file */	
	FILE *output = fopen(file_name, "w");
	if(!output) {
		printf("invalid");
		exit(255);
	}

	unsigned char finalMessage[cipher_len + 16];
	memcpy(finalMessage, tag, 16);
	memcpy(finalMessage+16, ciphertext, cipher_len);
    fwrite(finalMessage, cipher_len + 16, 1, output);
	fclose(output);
}

/* return lines */
char decrypt_log(unsigned char *shortToken, char* file_name, unsigned char** output) {
	//Hash token to 256 bits
	unsigned char token[32];
	hashKey(shortToken, token);

	FILE *input = fopen(file_name, "r");

	if(!input) {
		unsigned char* empty = malloc(1);
		if(empty == NULL) {
			error("Could not allocate");
		}
		empty[0] = '\0';
		*output = empty;
		return true;
	}

	int input_len;
	fseek(input, 0L, SEEK_END);
    input_len = ftell(input);
    rewind(input);

	//Hold the file data
	unsigned char ciphertext[input_len];
	unsigned char tag[16];
	//Read the file payload into the string. Last 16 bytes are tag
    int tag_len = fread(tag, sizeof(char), 16, input);
    int cipher_len = fread(ciphertext, sizeof(char), input_len-16, input);
   	fclose(input);

   	EVP_CIPHER_CTX *context;
	//Create a string to hold the decrypted file. Same size as encrypted data
   	unsigned char* plaintext = malloc(input_len+16);
   	if(plaintext == NULL) {
   		error("Could not allocate");
   	}
   	unsigned const char iv[12] = {0};
	int plain_len;
	int temp;

	context = EVP_CIPHER_CTX_new();
	EVP_DecryptInit_ex(context, EVP_aes_256_gcm(), NULL, token, iv);
	EVP_DecryptUpdate(context, plaintext, &plain_len, ciphertext, cipher_len);
	//Expected tag
    EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_TAG, 16, tag);
	int valid = EVP_DecryptFinal_ex(context, plaintext + plain_len, &temp);
	plain_len += temp;

	EVP_CIPHER_CTX_free(context);
	*output = plaintext;
	//return plaintext;
	return valid > 0;
}