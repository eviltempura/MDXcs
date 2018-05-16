#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
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

#include <stdint.h>
#include <ctype.h>

#define KEY_LEN 32
#define IV_LEN 16

/*Prototypes*/
int encrypt(EVP_CIPHER_CTX *ctx, unsigned char *pltxt,
            int pltxt_len, unsigned char *key, 
            unsigned char *iv, unsigned char *ciphertext);
int decrypt(EVP_CIPHER_CTX *ctx, unsigned char *ciphertext,
                      int ciphertext_len, unsigned char *key,
                      unsigned char *iv, unsigned char *plaintext);
void handleErrors(int B);
//void sha128_hash(const unsigned char *msg, size_t msg_len, unsigned char *digest, unsigned int *digest_len, int *B);
void parse_cmdline(int argc, char **argv, EVP_CIPHER_CTX *ctx);

/********************************************/

/*void sha128_hash(const unsigned char *msg,
                  size_t msg_len, unsigned char *digest,
                  unsigned int *digest_len, int *B) {
  EVP_MD_CTX *mdctx;

  if ((mdctx = EVP_MD_CTX_create()) == NULL) {
    handleErrors(B);
  }

  if (1 != EVP_DigestInit_ex(mdctx, EVP_sha128(), NULL)) {
    handleErrors(B);
  }

  if (1 != EVP_DigestUpdate(mdctx, msg, msg_len)) {
    handleErrors(B);
  }

  if (1 != EVP_DigestFinal_ex(mdctx, digest, digest_len)) {
    handleErrors(B);
  }

  EVP_MD_CTX_destroy(mdctx);
}*/

int encrypt(EVP_CIPHER_CTX *ctx, unsigned char *pltxt,
            int pltxt_len, unsigned char *key, 
            unsigned char *iv, unsigned char *ciphertext) {

    int len;
    int ciphertext_len;

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
      handleErrors(0);
    }

    if (1 != EVP_EncryptUpdate(ctx, ciphertext, &len, pltxt, pltxt_len)) {
      handleErrors(1);
    }

    ciphertext_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
      handleErrors(2);
    }

    ciphertext_len += len;

    return ciphertext_len;
}

int decrypt(EVP_CIPHER_CTX *ctx, unsigned char *ciphertext,
                      int ciphertext_len, unsigned char *key,
                      unsigned char *iv, unsigned char *plaintext) {
    int len;
    int pltxt_len;
    int ret;

    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
      handleErrors(3);
    }

    if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
      handleErrors(4);
    }
    pltxt_len = len;

    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    if (ret > 0) {
      pltxt_len += len;
      return pltxt_len;
    } else {
      return -1;
    }
}

void handleErrors(int B) {
  printf("invalid\n");
  exit(255);
}




void parse_cmdline(int args, char **argsa, EVP_CIPHER_CTX *ctx) {

  int opt = -1;

  int i; //For any iterations
  size_t len;

  int B = 0, T = 0, K = 0, E = 0, G = 0, A = 0, L = 0, R = 0;


  unsigned char *plaintext = NULL; //BUFFER FOR ANY PLAINTEXT GOTTEN FROM LOG
  unsigned int plaintext_len; //Populated when decrypting to know how much plaintext to read!

  unsigned char *ciphertext = NULL; //BUFFER FOR ANY CIPHERTEXT TO BE WRITTEN TO FILE
  unsigned int ciphertext_len; //Populated when encrypting to know how much ciphertext to read!

  unsigned char *path = NULL; //This is the logfile or -B file
  unsigned char *name = NULL;
  unsigned char *real_key = NULL; //Not cut down to 32, still checks authenticity

  unsigned char real_time[10 + 1];
  unsigned int room_number, time;
  unsigned char real_room_num[10 + 1];

  unsigned char iv[IV_LEN]; //can't be bigger
  unsigned char key[KEY_LEN];//store the value of the key they use to access the file
 
  memset(iv, 0, IV_LEN);
  memset(key, 0, KEY_LEN); 

  memset(real_room_num, 0, 11);
  memset(real_time, 0, 11); 

  //pick up the switches
  while ((opt = getopt(args, argsa, "T:K:E:G:ALR:B:")) != -1) {
    switch(opt) {
      case 'B':
        if (T == 1 || K == 1|| E == 1 || G == 1
           || A == 1 || L == 1 || R == 1 || args > 3) {
          handleErrors(5); //this invocation only takes a -B tag
          //Pointer to B to ensure that it is only ever called once
          //on a given invocation of program
        }
        B = 1;

        len = strlen(optarg); 
        path = malloc(len + 1);
        for(i = 0; i < len; i++){
          //checks to make sure each char of the log filename is valid
          if(!isdigit(optarg[i]) && 
              !isalpha(optarg[i]) && 
              optarg[i] != '.' && 
              optarg[i] != '/' && 
              optarg[i] != '_' && 
              optarg[i] != '\0') {
            handleErrors(6);
          }
          path[i] = optarg[i];
        }
        path[i] = '\0';

        
        

        break;

      case 'T':
        //timestamp
        if (B) {
          handleErrors(7);
        }        

        T = 1;

        len = strlen(optarg);
        if (len > 10) {
          handleErrors(8);
        }

        time = strtol(optarg, NULL, 10); //time = next arguement after -T, turns optarg from string into int **may need to check if they input a string or something incorrect
        //Check to make sure time is within the bounds. Integer overflow.
        if (time < 1 || time > 1073741823) {
          handleErrors(9);
        }
        
        memset(real_time, 0, 11);

        for (i = 0; i < len; i++) {
          if (!isdigit(optarg[i])) {
            handleErrors(10);
          }
          real_time[i] = optarg[i];
        }
        real_time[i] = '\0';

        break;

      case 'K':
        if (B) {
          handleErrors(11);
        }
        K = 1;

        free(real_key);
        real_key = NULL;
        
        //go through each character of the key being passed in, if its not a alphabetic char or its not a number then the key is invalid.
        //if it is valid then store that value in the key
        
        len = strlen(optarg);
        real_key = malloc(len +1);

       for(i = 0; i < KEY_LEN; i++){
        if (i < len) {
          if(!isdigit(optarg[i]) && !isalpha(optarg[i])) { //if non-numeric and non-alphabetic then fail can be one or the other
            handleErrors(12);
          }
          if (i < IV_LEN) {
            iv[i] = optarg[i];
          }
          key[i] = optarg[i];
          real_key[i] = optarg[i];
        } else { //Pad spaces up to appropriate lengths
          if (i < IV_LEN) {
            iv[i] = ' ';
          }
          key[i] = ' ';
        }


      } 
      for (; i < len; i++) {
        if (!isdigit(optarg[i]) && !isalpha(optarg[i])) {
          handleErrors(13);
        }
        real_key[i] = optarg[i];
      }
      real_key[i] = '\0';

        break;

      case 'A':
        //check if we have already seen -L
        if (L || B) {
          handleErrors(14);
        }
        A = 1;

        break;

      case 'L':
        //departure
        //check if we have alerady seen -A
        if (A || B) {
          handleErrors(15);
        }
        L = 1;
        break;

      case 'E':
        //employee name
        //check to see if command -G was input
        if (G || B) {
          handleErrors(16);
        }
        E = 1;

        free(name);
        name = NULL;

        len = strlen(optarg);
        name = malloc(len + 1);
        for(i = 0; i < len; i++){
          if(!isalpha(optarg[i])){
            handleErrors(17);
          }
          name[i] = optarg[i];
        }
        name[i] = '\0';
        break;

      case 'G':
        //guest name
        if (E || B) {
          handleErrors(18);
        }
        G = 1;
        
        free(name);
        name = NULL;

        len = strlen(optarg);
        name = malloc(len + 1);
        for(i = 0; i < len; i++){
          if(!isalpha(optarg[i])){
            handleErrors(41);
          }
          name[i] = optarg[i];
        }
        name[i] = '\0';

        break;

      case 'R':

        if (B) {
          handleErrors(19);
        }
        R = 1;

        memset(real_room_num, 0, 11);

        room_number = strtol(optarg, NULL, 10);

        if (room_number) { //Don't need to check whether true zero; i.e., 0000000 or 000
          if (room_number < 0 || room_number > 1073741823) {
            handleErrors(20);  //Bad input
          }
          len = strlen(optarg);
					i = 0;
					while(optarg[i] == '0') {
						i++;
					}
					int j = i;
          for (; j < len; ++j) {
            if (!isdigit(optarg[j])) {
              handleErrors(21);
            }
            real_room_num[j - i] = optarg[j];
          }
          real_room_num[j - i] = '\0';
        } else { //check if true zero or error case
          if (optarg[0] == '0') { //Still potential for true zero
            len = strlen(optarg);
            i = 1;
            while (optarg[i] == '0') { //while the next character is a zero
              i++;
            } //now i is index of nonzero char
            if (len == i) {
              real_room_num[0] = '0'; //TRUE ZERO
              real_room_num[1] = '\0';
            } else {
              handleErrors(22);
            }
          } else {
            handleErrors(23);
          }
        }

       
        break;


      default:
        //unknown option, leave
        handleErrors(24);
        break;
    }

  }
  //Since we're here, we know everything's good with the format of the commands

  // now we need to get the log file and check the file to see if the commands do not invoke other errors

  //pick up the positional argument for log path
  if (optind == args - 1) { //Good case, log file provided
    len = strlen(argsa[optind]);
    path = malloc(len + 1);
    for(i = 0; i < len; i++){
      //checks to make sure each char of the log filename is valid
      if(!isdigit(argsa[optind][i]) && 
          !isalpha(argsa[optind][i]) && 
          argsa[optind][i] != '.' && 
          argsa[optind][i] != '/' && 
          argsa[optind][i] != '_' && 
          argsa[optind][i] != '\0') {
        handleErrors(25);
      }
      path[i] = argsa[optind][i];
    }
    path[i] = '\0';

    //Check whether command has all of the necessary components
    if (!(T && K && (E || G) && (A || L)) &&
		!(B && !T && !K && !E && !G && !A && !L && !R)) {
      handleErrors(26);
    } 
    FILE *fp;

		if (B) { //begin batch processing, will return
        if ((fp = fopen((char *)path, "r")) == NULL) {
          handleErrors(42);
        }
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        rewind(fp);

        char *command = NULL;
        
        size_t buff_len;
        ssize_t com_len; 

        while ((com_len = 
                getline(&command, &buff_len, fp)) > 0) {
          //command is now populated
          command[com_len - 1] = '\0';
          //strips newline
          //use clever sscanf to populate a new argc and argv to call this same function with
          char *t = "./logappend ";
          int tlen = strlen(t);

          char *comb = malloc(tlen + com_len + 1);
          strcpy(comb, t);
          strcat(comb, command);
          if (strstr(comb, " -B") || strstr(comb, "-B ")) {
            printf("invalid\n");
          } else {
            system(comb);
          }
        }

        //free some more stuff
        free(path);
        path = NULL;
        free(command);
        command = NULL;
        fclose(fp);

        return; //We're finished with batch processing
		}	

    //At this point, you know the command has all the proper arguments
    //now it is time to check for semantics

    //check if file exists, might need option "b" since the file should be binary because it's encrypted
  
    if ((fp = fopen((char *) path, "r")) != NULL) {
      unsigned char type[2];
      if (E) {
        type[0] = 'E';
      } else {
        type[0] = 'G';
      }
      type[1] = '\0';
      //You're at the index of
      // first character of ciphertext
     
      
      int hasArrived = 0; //if the person has arrived to the gallery
      
      unsigned char *must_exit = NULL;
      int isInARoom = 0; //if the person is in a room
      unsigned char *curr_time = NULL; //get the time of the current line
      unsigned char *curr_name = NULL;
      unsigned char typeOfPerson[2]; //type of person either E or G
      unsigned char arriveOrLeave[2]; // A or L
      unsigned char *room_num = NULL; // store the value of the current line room num
      unsigned char *curr_key = NULL; //not really needed for this part 
      
      fseek(fp, 0, SEEK_END);
      ciphertext_len = ftell(fp);
      rewind(fp);
      //file exists
      
      //fill ciphertext
      ciphertext = malloc(ciphertext_len);
      fread(ciphertext, 1, ciphertext_len, fp);
      fclose(fp);//done reading

      unsigned char *scapegoat = malloc(ciphertext_len);
      memset(scapegoat, 0, ciphertext_len);

      int dummy;
      if (-1 == 
          (dummy = decrypt(ctx, ciphertext,
            ciphertext_len, key, iv, scapegoat))) {
        handleErrors(27);
      }

      plaintext_len = (unsigned int) dummy;
      plaintext = malloc(plaintext_len);
      memcpy(plaintext, scapegoat, plaintext_len);

      free(scapegoat);
      scapegoat = NULL;
      //plaintext populated

      i = 0;
      while (i < plaintext_len) {
        arriveOrLeave[0] = plaintext[i++];
        arriveOrLeave[1] = '\0';
        i++;//Space

        typeOfPerson[0] = plaintext[i++];
        typeOfPerson[1] = '\0';
        i++;//Space

        len = 0;
        while (plaintext[i + len] != ' ') {
          len++;
        }
        curr_key = malloc(len + 1);
        memcpy(curr_key, plaintext + i, len);
        i += len + 1;
        curr_key[len] = '\0';

        if (strcmp((char *) curr_key, (char *) real_key)) {
          handleErrors(28);
        }

        len = 0;
        while(plaintext[i + len] != ' ') {
          len++;
        }
        curr_time = malloc(len + 1);
        memcpy(curr_time, plaintext + i, len);
        i += len + 1;
        curr_time[len] = '\0';

        if (time <= strtol((char *) curr_time, NULL, 10)) {
          handleErrors(29);
        }

        len = 0;
        while(plaintext[i + len] != ' ' 
            && plaintext[i + len] != '\0') {
          len++;
        }
        curr_name = malloc(len + 1);
        
        memcpy(curr_name, plaintext + i, len);

        if (plaintext[i + len] == ' ') {
          i += len + 1;
          curr_name[len] = '\0';
          len = 0;
          while(plaintext[i + len] != '\0') {
            len++;
          }
          room_num = malloc(len + 1);
          memcpy(room_num, plaintext + i, len);
          i += len + 1;
          room_num[len] = '\0';

        } else {
          curr_name[len] ='\0';
          i += len + 1;
        }
        
        if (!strcmp((char *)curr_name,(char *) name) 
          && typeOfPerson[0] == type[0]) {
         
          if (room_num == NULL) {
            if (arriveOrLeave[0] == 'A') {
              hasArrived = 1;
              isInARoom = 0;
            } else {
              hasArrived = 0;
              isInARoom = 0;
            }
          } else {
            if (arriveOrLeave[0] == 'A') {
              hasArrived = 1;
              isInARoom = 1;
              
              free(must_exit);
              must_exit = NULL;

              must_exit = malloc(len + 1);
              memset(must_exit, 0, len + 1);
              memcpy(must_exit, room_num, len);

            } else {
              hasArrived = 1;
              isInARoom = 0;
            }
          }
        }
        //free stuff
      free(curr_name);
      curr_name = NULL;
      free(curr_time);
      curr_time = NULL;
      free(curr_key);
      curr_key = NULL;
      free(room_num);
      room_num = NULL;
      } 

      if (hasArrived && isInARoom) {
          if (A) {
            handleErrors(30);
          }
          //So you're trying to leave, you need to have a room
          if (!R) {
            handleErrors(31);
          }
          //You have a room to leave, is it the one you're in?
          if (strcmp((char *)must_exit, (char *)real_room_num)) {
            handleErrors(32);
          }
         //You're good to leave this room
      } else if (hasArrived && !isInARoom) {
        if (A) {
          //you want to enter a room, did you provide a room?
          if (!R) {
            handleErrors(33);
          }
        } else {
          //you want to leave, but you can only leave the gallery
          if (R) {
            handleErrors(34);
          }
        }
        //You're good to enter this room or leave the gallery
      } else { //you have not arrived again after leaving gallery
        //you may only re-enter gallery
        if (R || L) {
          handleErrors(35);
        }
        //You're good to enter the gallery again
      }

      free(must_exit);
      must_exit = NULL;
      
      //command is good, now calculate new plaintext length
      i = 4;

      len = strlen((char *) real_key);
      i += len + 1;

      len = strlen((char *) real_time);
      i += len + 1;

      len = strlen((char *) name);
      i += len + 1;
      
      if (R) {
        len = strlen((char *) real_room_num);
        i += len + 1;
      } 


      scapegoat = malloc(plaintext_len + i);
      memcpy(scapegoat, plaintext, plaintext_len);

      i = plaintext_len;
      if (A) {
        scapegoat[i++] = 'A';
      } else {
        scapegoat[i++] = 'L';
      }
      scapegoat[i++] = ' ';

      scapegoat[i++] = type[0];

      scapegoat[i++] = ' ';

      len = strlen((char *) real_key);
      memcpy(scapegoat + i, real_key, len);
      i += len;

      scapegoat[i++] = ' ';

      len = strlen((char *) real_time);
      memcpy(scapegoat + i, real_time, len);
      i += len;

      scapegoat[i++] = ' ';

      len = strlen((char *) name);
      memcpy(scapegoat + i, name, len);
      i += len;

      if (R) {
        scapegoat[i++] = ' ';
        len = strlen((char *) real_room_num);
        memcpy(scapegoat + i, real_room_num, len);
        i += len;
      }

      scapegoat[i++] = '\0';

      plaintext_len = i; //time to encrypt again

      free(ciphertext);
      ciphertext = NULL;

      ciphertext_len = plaintext_len + 
                        (16 - (plaintext_len % 16));
      ciphertext = malloc(ciphertext_len);

      encrypt(ctx, scapegoat, plaintext_len,
              key, iv, ciphertext);

      //open a file for writing
      if ((fp = fopen((char *) path, "w")) == NULL) {
        handleErrors(36);
      }

      fwrite(ciphertext, 1, ciphertext_len, fp);

      free(scapegoat);
      scapegoat = NULL;
      
      //DONE with logappend
    } else { 
      //create a new file for writing
     
      //THAT MEANS THIS IS THE FIRST COMMAND FOR THIS LOG
      //it has to be a valid first command, which means
      //there cannot be an R or L provided
      if (R || L) {
        handleErrors(37);
      }
      //time to encrypt... I think we should just store the encrypted command in the log file
      //and then terminate each plaintext with a newline explicitly to delineate them for reading

      i = 4;

      len = strlen((char *) real_key);
      i += len + 1;
      
      len = strlen((char *) real_time);
      i += len + 1;
      
      len = strlen((char *) name);
      i += len + 1;

      plaintext_len = i;

      free(plaintext);
      plaintext = NULL;

      plaintext = malloc(plaintext_len);

      memset(plaintext, 'A', 1);
      memset(plaintext + 1, ' ', 1);
      if (E) {
        memset(plaintext + 2, 'E', 1); 
      } else {
        memset(plaintext + 2, 'G', 1); 
      }
      memset(plaintext + 3, ' ', 1);
      i = 4;

      len = strlen((char *) real_key);
      memcpy(plaintext + i, real_key, len);
      i += len;
      memset(plaintext + i, ' ', 1);
      i++;

      len = strlen((char *) real_time);
      memcpy(plaintext + i, real_time, len);
      i += len;
      memset(plaintext + i, ' ', 1);
      i++;

      len = strlen((char *) name);
      memcpy(plaintext + i, name, len);
      i += len;
      memset(plaintext + i, '\0', 1);
      i++;

      //Encryption time now that plaintext is filled properly
      ciphertext_len = plaintext_len + (16 - (plaintext_len % 16));

      ciphertext = malloc(ciphertext_len);

      ciphertext_len = encrypt(ctx, plaintext, plaintext_len, 
              key, iv, ciphertext);

      //Now just write this ciphertext to the logfile path
      
      if ((fp = fopen((char *) path, "w")) == NULL) {
        handleErrors(38);
      }
      
      fwrite(ciphertext, 1, ciphertext_len, fp);
      
      //DONE WITH THE EASY CASE
    }

    //free some stuff
    fclose(fp);
   
    free(name);
    name = NULL;
    free(real_key);
    real_key = NULL;
    free(ciphertext);
    ciphertext = NULL;
    free(plaintext);
    plaintext = NULL;
    free(path);
    path = NULL;

  } else {
    handleErrors(39); //Incorrect arguments provided
  }

}



int main(int argc, char **argv) {
	
  EVP_CIPHER_CTX *ctx;
  if(!(ctx = EVP_CIPHER_CTX_new())){
    handleErrors(40);
  }

  parse_cmdline(argc, argv, ctx);

  EVP_CIPHER_CTX_free(ctx);
  return 0;
}
