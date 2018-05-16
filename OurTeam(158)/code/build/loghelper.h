#define false 0
#define true 1
//Length of line not including name
#define FORMAT_LENGTH 29 
#define DEBUG 0

void handleErrors();

void error(const char* msg);

void parseEvent(char* audit, int start, int end, int* time, char* current, char* guest, char** name, char* arriving, int* room);

char isAlphabetical(char* str);

char isAlphanumeric(char* str);

int getMostRecentEventIndex(char* auditLog, int* eventBounds, int numEvents, char* targetName, char targetGuest);

void encrypt_write_log(const unsigned char* shortToken, const char* file_name, const unsigned char* lines, const unsigned char *append);

int split_message(char* str, int** returnarr);

char decrypt_log(unsigned char *longToken, char* file_name, unsigned char** output);