#include"helper.h"

int main(int argc, char *argv[])
{
    int opt = -1;
    int is_good = -1;
    int len;
    long int temp;//temp long number
    int result;
    int unim =0;
    int if_t=0;
    int if_k=0;
    unsigned char userKey[16];
    char timestamp[16];
    char name[256];
    char room[128];
    char aorl[3];//arrive or leave
    char eorg[3];//employee or guest
    char log[128];

   
    FILE *file;
    int choice;
    memset(room,0x0,sizeof(room));
    memset(name,0x0,sizeof(name));
    memset(aorl,0x0,sizeof(aorl));
    memset(eorg,0x0,sizeof(eorg));
    memset(timestamp,0x0,sizeof(timestamp));
    memset(userKey,0x0,sizeof(userKey));
    memset(log,0x0,sizeof(log));
   
    while ((opt = getopt(argc, argv, "T:K:E:G:ALR:B:")) != -1)
    {
        switch(opt)
        {
        case 'B':
       unim=1;
            break;
        case 'T':
        if_t=1;
       
            len=strlen(optarg);
            for(int i=0; i<len; i++)
            {
                if(isdigit(optarg[i])==0)
                {
                    printf("%s\n","invalid");
                    return (255);
                }
            }
            temp=atol(optarg);
            if(temp>1073741823 || temp<1)
            {
                printf("%s\n","invalid");
                return (255);
            }
            snprintf(timestamp, sizeof(timestamp), "%ld",temp);
            break;
        case 'K':
        if_k=1;
            len= strlen(optarg);
            for(int i=0; i<len; i++)
            {
                if( (isalpha(optarg[i])==0) && (isdigit(optarg[i])==0))
                {
                    printf("%s\n","invalid");
                    return (255);
                }
            }
            MD5_CTX ctx;
            unsigned char outmd[16];
            memset(outmd,0,sizeof(outmd));
            MD5_Init(&ctx);
            MD5_Update(&ctx,optarg,len);
            MD5_Final(outmd,&ctx);
            for(int i=0; i<16; i++)
            {
                userKey[i]=outmd[i];
            }
            //secret token
            break;
        case 'A':
            if(aorl[0]!='\0'){
                 printf("%s\n","invalid");
                    return (255);
            }

            aorl[0]='-';
            aorl[1]='A';
            //arrival
            break;
        case 'L':
        if(aorl[0]!='\0'){
                 printf("%s\n","invalid");
                    return (255);
            }
            aorl[0]='-';
            aorl[1]='L';
            break;
        case 'E':
        if(eorg[0]!='\0'){
                 printf("%s\n","invalid");
                    return (255);
            }
            eorg[0]='-';
            eorg[1]='E';
            strncpy(name,optarg,sizeof(name)-1);
            break;
        case 'G':
        if(eorg[0]!='\0'){
                 printf("%s\n","invalid");
                    return (255);
            }
            eorg[0]='-';
            eorg[1]='G';
            strncpy(name,optarg,sizeof(name)-1);
            break;
        case 'R':
            len=strlen(optarg);
            for(int i=0; i<len; i++)
            {
                if(isdigit(optarg[i])==0)
                {
                    printf("%s\n","invalid");
                    return (255);
                }
            }
            temp=atol(optarg);  
            if(temp>1073741823 || temp<0)
            {
                printf("%s\n","invalid");
                return (255);
            }
            snprintf(room, sizeof(room), "%ld",temp);
            break;
        default:
            break;
        }
    }
    if(unim==1){
        if(argc==3){
             printf("unimplemented\n");
            return 0;

        }
        else{
            printf("invalid\n");
            return 255;
        }
    }

    if(if_k==0){
        printf("invalid\n");
        return 255;
    }
     if(if_t==0){
        printf("invalid\n");
        return 255;
    }


    if(optind < argc)
    {
        if(exists(argv[optind]))//log exists
        {
            choice=2;
               strcpy(log,argv[optind]);
            file=fopen(argv[optind],"r+");
            if(file==NULL)
            {
                printf("%s\n","file error");
                return 0;
            }
        }
        else
        {
            choice=1;//log doesn't exists
            strcpy(log,argv[optind]);
            // file=fopen(argv[optind],"w+");
            // if(file==NULL)
            // {
            //     printf("%s\n","file error");
            //     return 0;
            // }
        }
    }
    if(choice==1)
    {
          //printf("log_name1\n%s\n",log );
        int res2=create_log(timestamp,userKey,eorg,name,aorl,room,log);
        return res2;

    }
    else if(choice==2)
    {
       // printf("log_name2\n%s\n",log );
        int res2 =append_log(timestamp,userKey,eorg,name,aorl,room,file,log);
         return res2;
       
    }
    // if(file!=NULL)
    // {
    //     fclose(file);
    // }
      //printf("%s\n","here233");
    return 0;
}



