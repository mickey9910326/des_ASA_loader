// use MinGW ti compile
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <getopt.h>


#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "rs232.h"

int CPORT_NR = 11;
int BDRATE = 115200;
long unsigned int temp;
char mode[]={'8','N','1',0};
DCB dcb;
DWORD dwCommEvent;
DWORD dwModemStatus;
// LPCOMMPROP lpCommProp;
COMMPROP ss;
LPDWORD lpModemStat;
int main(int argc, char **argv) {
	char tmp_name[25];
	int chd;
    char* endptr;
    while (1) {
        static struct option long_options[] =
          {
            {"help",  no_argument,       0, '?' },
            {"port",  required_argument, 0, 'p'},
            {"hex" ,  required_argument, 0, 'h'},
            {0, 0, 0, 0}
          };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        chd = getopt_long (argc, argv, "p:h:",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (chd == -1)
          break;

        switch (chd)
          {
          case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
              break;
            printf ("option %s", long_options[option_index].name);
            if (optarg)
              printf (" with arg %s", optarg);
            printf ("\n");
            break;

          case 'h':
		  	strcpy(tmp_name,optarg);
            break;

          case 'p':
  			CPORT_NR = (int)strtoimax(optarg,&endptr,10)-1;
            break;

          case '?':
            printf("\nUsage: des_ASA_loader [--port <com>] [--hex <file_name>]\n");
			printf("  --port <com>\t\t:: Use desinated port <com>\n");
			printf("  --hex <file_name>\t:: Load <file_name>\n");
			return 0;
            break;

          default:
            abort ();
          }
      }

	unsigned char start[12] = {
		0xfc,0xfc,0xfc,0xfa,0x01,
		0x00,0x04,
		0x74,0x65,0x73,0x74,0xc0};
	// chk
    unsigned char end[8] = {0xfc,0xfc,0xfc,0xfc,0x01,0x00,0x00,0x00};

	unsigned char header[5] = {0xfc,0xfc,0xfc,0xfc,0x01};
	unsigned char length[2] = {0x00,0x10};
	unsigned char tt_data[265] = {0xfc,0xfc,0xfc,0xfc,0x01};
	// header (5bytes) -> length (2byte)
	char file_name[25];
	strcpy(file_name,tmp_name);
    FILE *fp;

    printf("Enter the name of file you wish to see\n");
    // fgets(file_name, sizeof(file_name), stdin);
    // file_name[strlen(file_name)-1] = 0x00;

    fp = fopen(file_name,"rb"); // read binary mode

    if( fp == NULL ) //error checking
    {
        perror("Error while opening the file.\n");
        exit(EXIT_FAILURE);
    }

    printf("The contents of %s file are :\n", file_name);
    // RS232_SendBuf(CPORT_NR,start,12);
    // Sleep( 100 );
	if(RS232_OpenComport(CPORT_NR, BDRATE, mode)) {
		printf("Can not connect to COM%d\n",CPORT_NR+1);
        return 0;
	}

    uint8_t status=1,c;
    uint8_t error=0,line_bytes,line_count;
    uint8_t checksum = 0;
    uint8_t data;
    uint16_t data_count = 0;
    uint8_t tri_data[257];
    uint8_t data_cc = 0;

	RS_SetCommMask(CPORT_NR,EV_RXCHAR);


	RS232_SendBuf(CPORT_NR,start,12);
	Sleep(100);
	RS_WaitCommEvent(CPORT_NR, &dwCommEvent,NULL);
printf("QQ\n");
	// RS_GetCommStatus(CPORT_NR, &ss);
	// RS_WaitCommEvent(CPORT_NR, &dwCommEvent);
	unsigned char tempa;
	for (int i = 0; i < 12; i++) {
		RS232_PollComport(CPORT_NR,&tempa,1);
	}
	RS_SetCommMask(CPORT_NR,EV_RXCHAR  |EV_TXEMPTY);

    while (error==0) {
        switch (status) {
            case 1:
                c = fgetc(fp);
                if ( c != ':' ) {
                    printf("error\n");
                    status = 10;
                } else {
                    status = 2;

                }
                break;
            case 2:
                fscanf(fp,"%02X", &data);
                if ( data == 0x00 ) {
                    error = 1;
                    status = 99;
                } else {
                    line_bytes = data;
                    line_count = 0;
                    status = 3;
                }
                break;
            case 3:
                fscanf(fp,"%02X", &data);
                fscanf(fp,"%02X", &data);
                status = 4;
                break;
            case 4:
                fscanf(fp,"%02X", &data);
                status = 5;
                break;
            case 5:
                for (uint8_t i = 0; i < line_bytes; i++) {
                    fscanf(fp,"%02X", &data);
                    checksum+=data;
                    data_count++;
                    tri_data[data_count-1] = data;
                    if (data_count==256) {
                        tri_data[data_count]=checksum;
                        for (uint16_t j = 0; j <= data_count; j++) {
                            printf("%02X ",tri_data[j]);
                            if ((j+1)%16==0) {
                                printf("\n");
                            }
                        }
                        length[1] = data_count%256;
                        length[0] = data_count/256;
                        memcpy(tt_data+5,length,2*sizeof(uint8_t));
                        memcpy(tt_data+7,tri_data,(data_count+1)*sizeof(uint8_t));
						// RS_SetCommMask(CPORT_NR,EV_RXCHAR);
						// RS_WaitCommEvent(CPORT_NR, &dwCommEvent,NULL);
						// RS_GetCommModemStatus(CPORT_NR, &dwModemStatus);

						RS232_SendBuf(CPORT_NR,tt_data,data_count+8);
						Sleep(100);
						RS_WaitCommEvent(CPORT_NR, &dwCommEvent,NULL);



                        data_cc++;
                        printf("\n%d ==================================================\n",data_cc);
                        data_count=0;
                        checksum=0;
                    }
                }
                status = 6;
                break;
            case 6:
                fscanf(fp,"%02X", &data);
                status = 7;
                break;
            case 7:
                fscanf(fp,"%02X", &data);
                status = 1;
                break;
            case 10:
                fscanf(fp,"%02X", &data);
                status = 1;
                break;

        }
    }
    tri_data[data_count]=checksum;
    for (uint16_t j = 0; j <= data_count; j++) {
        printf("%02X ",tri_data[j]);
        if ((j+1)%16==0) {
            printf("\n");
        }
    }
    data_cc++;
    printf("\n%d ==================================================\n",data_cc);
    length[1] = data_count%256;
    length[0] = data_count/256;
    memcpy(tt_data+5,length,2*sizeof(uint8_t));
    memcpy(tt_data+7,tri_data,(data_count+1)*sizeof(uint8_t));
    // Sleep( 10 );
	// RS_WaitCommEvent(CPORT_NR, &dwCommEvent,NULL);
	// RS_GetCommModemStatus(CPORT_NR, &dwModemStatus);
    RS232_SendBuf(CPORT_NR,tt_data,data_count+8);
	Sleep(100);
	RS_WaitCommEvent(CPORT_NR, &dwCommEvent,NULL);

    // Sleep( 10 );
	// RS_WaitCommEvent(CPORT_NR, &dwCommEvent,NULL);
	// RS_GetCommModemStatus(CPORT_NR, &dwModemStatus);
    RS232_SendBuf(CPORT_NR,end,8);
	Sleep(100);
	RS_WaitCommEvent(CPORT_NR, &dwCommEvent,NULL);

	for (int i = 0; i < 12; i++) {
		RS232_PollComport(CPORT_NR,&tempa,1);
	}
    fclose(fp);

    return 0;
}
