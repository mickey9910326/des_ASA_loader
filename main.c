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

char* VERISON = "v2.0";

uint8_t parse_hex_data_iter(FILE* fp, void* data_p, uint16_t* data_num, uint8_t* chksum);
void print_hex_data(void* data_p, uint16_t data_count);

int main(int argc, char **argv) {
	uint8_t error = 0;
	int CPORT_NR = 11;
	int BDRATE = 115200;
	char mode[]={'8','N','1',0};

	char tmp_name[256];
	int ch;
	int do_verbose = 0;
	int do_debug   = 0;

	// handle option argv
	struct option long_options[] =
	{
		{"verbose" , no_argument      , &do_verbose, 1},
		{"debug"   , no_argument      , &do_debug  , 1},
		{"version" , no_argument      , NULL       , 'v'},
		{"help"    , no_argument      , NULL       , '?'},
		{"port"    , required_argument, NULL       , 'p'},
		{"hex"     , required_argument, NULL       , 'h'},
		{0, 0, 0, 0}
	};
    while (1) {
        /* getopt_long stores the option index here. */
        int option_index = 0;

        ch = getopt_long (argc, argv, "p:h:",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (ch == -1)
        	break;

        switch (ch) {
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
		 		CPORT_NR = (int)strtoimax(optarg,NULL,10)-1;
		    	break;

			case 'v':
				printf("des_ASA_loader version is %s \n",VERISON);
				return 0;
		        break;

	        case '?':
		        printf("Usage: des_ASA_loader [--port <com>] [--hex <file_name>]\n");
				printf("  --port <com>         Use desinated port <com>\n");
				printf("  --hex <file_name>    Load <file_name>\n");
				return 0;
		        break;

	        default:
	        	abort ();
        }
    }

	// core
	unsigned char start[12] 	= {0xfc,0xfc,0xfc,0xfa,0x01,0x00,0x04,0x74,0x65,0x73,0x74,0xc0};
    unsigned char end[8] 		= {0xfc,0xfc,0xfc,0xfc,0x01,0x00,0x00,0x00};
	unsigned char tri_data[265] = {0xfc,0xfc,0xfc,0xfc,0x01};
	unsigned char chk[12]		= {0xfc,0xfc,0xfc,0xfc,0x01,0x00,0x04,'O','K','!','!','?'};
	unsigned char get_chk[12]	= {0xfc,0xfc,0xfc,0xfc,0x01};
	uint8_t hex_data[254];
	uint16_t data_count = 0;
	uint8_t chksum = 0;
	DWORD dwCommEvent;

	char file_name[256];
	strcpy(file_name,tmp_name);
	FILE *fp;
    fp = fopen(file_name,"rb"); // read binary mode
    if( fp == NULL ) { //error checking
		char buffer[256];
		sprintf(buffer, "User input PATH: %s\n", file_name);
		perror(buffer);
        perror("Cannot find file. Please check the file name.\n");
        exit(EXIT_FAILURE);
    }
	if(RS232_OpenComport(CPORT_NR, BDRATE, mode)) {
		printf("Can not connect to COM %d.\n",CPORT_NR+1);
		printf("Please check M128 is connected to USB and im program mode.");
		exit(EXIT_FAILURE);
	}

	printf("Start uploading %s to COM %d !\n", file_name, CPORT_NR+1);

	RS_SetCommMask(CPORT_NR,EV_RXCHAR);
	RS232_SendBuf(CPORT_NR,start,12);
	Sleep(100);
	RS_WaitCommEvent(CPORT_NR, &dwCommEvent,NULL);

	RS232_PollComport(CPORT_NR,get_chk,12);
	if (!strncpy(get_chk,chk,12)) {
		error = 1;
	}
	if (error) {
		printf("plz press reset bottom, and check mode is program mode\n");
		return 0;
	}
	do {
		parse_hex_data_iter(fp, hex_data, &data_count, &chksum);
		tri_data[5] = data_count/256;
		tri_data[6] = data_count%256;
        tri_data[6+data_count+1] = chksum;
		memcpy(tri_data+7,hex_data,data_count);
		RS232_SendBuf(CPORT_NR,tri_data,data_count+8);
		Sleep(30);
		if (do_verbose) {
			print_hex_data(hex_data, data_count);
			printf("chksum=%02X\n", chksum);
			printf("--------------------------------------\n");
		}
	} while(data_count==256);
	RS232_SendBuf(CPORT_NR,end,8);
	RS_WaitCommEvent(CPORT_NR, &dwCommEvent,NULL);
	RS232_PollComport(CPORT_NR,get_chk,12);
	if (!strncpy(get_chk,chk,12)) {
		error = 1;
	}

	if (error) {
		printf("WORNING : \n");
		printf("  Don't press reset bottom in programming!\n");
		printf("  Press the reset bottom and program again.\n");
		return 0;
	}
	
	printf("Upload %s successful !\n", file_name);

    fclose(fp);

    return 0;
}

uint8_t parse_hex_data_iter(FILE* fp, void* data_p, uint16_t* data_num, uint8_t* chksum) {
	#define DE_START 1
	#define DE_BYTES 2
	#define DE_ADDRESS 3
	#define DE_TYPE 4
	#define DE_DATA 5
	#define DE_CHKSUM 6
	#define GET_LINE_END 7

	// static long long int position = 0;
	static uint8_t status = 1;
	static uint8_t line_bytes;
	static uint8_t line_index;
	uint8_t res = 0;
	uint8_t error = 0;
	// printf("in_pos%lld\n", position);
	// char chk = fseek(fp,position,SEEK_SET);
	// printf("chk = %d\n",chk );
	*data_num = 0;
	*chksum = 0;

	char c;
	int int_data;
	while (error==0 && res ==0) {
        switch (status) {
            case DE_START:
                c = fgetc(fp);
                if ( c != ':' ) {
                    printf("error\n");
                    status = 10;
					error = 1;
                } else {
                    status = DE_BYTES;
                }
                break;
            case DE_BYTES:
                fscanf(fp,"%02X", &int_data);
                if ( int_data == 0x00 ) { //hnadle the last line
                    res = 1;
                } else {
                    line_bytes = int_data;
                    line_index = 0;
                    status = DE_ADDRESS;
                }
                break;
            case DE_ADDRESS:
                fscanf(fp,"%02X", &int_data);
                fscanf(fp,"%02X", &int_data);
                status = DE_TYPE;
                break;
            case DE_TYPE:
                fscanf(fp,"%02X", &int_data);
                status = DE_DATA;
                break;
            case DE_DATA:
				if((*data_num)==256) {
					res = 1;
					break;
				} else if (line_index<line_bytes) {
					fscanf(fp,"%02X", &int_data);
					((uint8_t*)data_p)[*data_num] = (uint8_t)int_data;
                    (*chksum)+=int_data;
                    (*data_num)++;
					line_index++;
					break;
				} else {
					status = DE_CHKSUM;
					break;
				}
				// printf("st = %d\n",status);

            case DE_CHKSUM:
                fscanf(fp,"%02X", &int_data);
                status = GET_LINE_END;
                break;
            case GET_LINE_END:
				// get line feed ch
                fscanf(fp,"%02X", &int_data);
                status = DE_START;
                break;
        }
    }
	return error;
}

void print_hex_data(void* data_p, uint16_t data_count) {
	for (uint16_t j = 0; j < data_count; j++) {
		printf("%02X ",((uint8_t*)data_p)[j]);
		if ((j+1)%16==0) {
			printf("\n");
		}
	}
}
