// use MinGW to compile
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <getopt.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <unistd.h>
#endif

#include "rs232.h"

#define ROUNDUP_QUOT(a,b) (((a) + (b) - 1) / (b))
// only work for positive numbers, and take care for overflow
#define PBSTR "============================================================"
#define PBSTR2 "                                                            "
#define PBWIDTH 60

#define VERISON "v2.3"

uint8_t parse_hex_data_iter(FILE* fp, void* data_p, uint16_t* data_num, uint8_t* chksum);
void print_hex_data(void* data_p, uint16_t data_count);
void print_progressbar(float percentage);



#ifdef _WIN32

// Used for set_cur_color() function
#define CONSOLE_FG_COLOR_RED	(FOREGROUND_RED)
#define CONSOLE_FG_COLOR_BLUE	(FOREGROUND_BLUE)
#define CONSOLE_FG_COLOR_GREEN	(FOREGROUND_GREEN)
#define CONSOLE_FG_COLOR_INST	(FOREGROUND_INTENSITY)

#define CONSOLE_BG_COLOR_RED	(BACKGROUND_RED)
#define CONSOLE_BG_COLOR_BLUE	(BACKGROUND_BLUE)
#define CONSOLE_BG_COLOR_GREEN	(BACKGROUND_GREEN)
#define CONSOLE_BG_COLOR_INST	(BACKGROUND_INTENSITY)

// Color available flag (global variable)
int is_use_color = 0;

// hAndle and Console setting
HANDLE hConsole;
WORD saved_attributes;

// Setup current Text paint color
void set_cur_color(WORD fg_color, WORD bg_color);
void restore_cur_color();

#else

// Used for set_cur_color() function
#define CONSOLE_FG_COLOR_RED		0
#define CONSOLE_FG_COLOR_BLUE		0
#define CONSOLE_FG_COLOR_GREEN	0
#define CONSOLE_FG_COLOR_INST		0

#define CONSOLE_BG_COLOR_RED		0
#define CONSOLE_BG_COLOR_BLUE		0
#define CONSOLE_BG_COLOR_GREEN	0
#define CONSOLE_BG_COLOR_INST		0

// Color available flag (global variable)
int is_use_color = 0;

void set_cur_color() {};
void restore_cur_color() {};

#endif

void sleep_ms(unsigned long);

int main(int argc, char **argv) {
	uint8_t error = 0;
	int CPORT_NR = 11;
	int BDRATE = 115200;
	char mode[]={'8','N','1',0};

	char tmp_name[256];
	int ch;
	int do_verbose = 0;
	int do_debug   = 0;
	int nouploading   = 0;

#ifdef _WIN32
	// Get console handle and record origin property
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	/* Save current attributes */
  	GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
  	saved_attributes = consoleInfo.wAttributes;
#endif

	// handle option argv
	struct option long_options[] =
	{
#ifdef _WIN32
        {"color"    , no_argument     , &is_use_color, 1},
#endif
		{"nouploading", no_argument   , &nouploading  , 1},
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
						printf("cmd_ASA_loader version is %s \n",VERISON);
						return 0;
			      break;

	        case '?':
				set_cur_color(CONSOLE_FG_COLOR_BLUE, 0);
		        printf("Usage: cmd_ASA_loader [--port <com>] [--hex <file_name>]\n");
				restore_cur_color();
				printf("  --port <com>         Use desinated port <com>\n");
				printf("  -p <com>             Same as --port <com>\n");
				printf("  --hex <file_name>    Load file <file_name>\n");
				printf("  -h <com>             Same as --hex <file_name>\n");
				printf("  --help               Show this messege\n");
				printf("  -?                   Same as --help\n");
				printf("  --verbose            Show external messege for debug usage\n");
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
	int32_t filesize;
	#ifdef __WIN32
	DWORD dwCommEvent;
	#endif

	char file_name[256];
	strcpy(file_name,tmp_name);
	FILE *fp;
    fp = fopen(file_name,"rb"); // read binary mode
    if( fp == NULL ) { //error checking
		char buffer[256];
		set_cur_color(CONSOLE_FG_COLOR_GREEN, CONSOLE_BG_COLOR_BLUE);
		sprintf(buffer, "User input PATH: %s\n", file_name);
		perror(buffer);
		set_cur_color(CONSOLE_FG_COLOR_RED, 0);
        perror("Cannot find file. Please check the file name.\n");
		restore_cur_color();
        exit(EXIT_FAILURE);
    }
	fseek(fp, 0L, SEEK_END);
	filesize = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	if(!nouploading)
	if(RS232_OpenComport(CPORT_NR, BDRATE, mode)) {
#ifndef _WIN32
		printf("Error Code : %d\n", errno);
#endif
		set_cur_color(CONSOLE_FG_COLOR_RED, 0);
		printf("Can not connect to COM %d.\n",CPORT_NR+1);
		printf("Please check M128 is connected to USB and im program mode.");
		restore_cur_color();
		exit(EXIT_FAILURE);
	}

	if(!nouploading){
		printf("Try to connect ASA_M128 with COM %d ...\n", CPORT_NR+1);
#ifdef __WIN32
		RS_SetCommMask(CPORT_NR,EV_RXCHAR);
#endif
		RS232_SendBuf(CPORT_NR,start,12);
		sleep_ms(100);
#ifdef __WIN32
		RS_WaitCommEvent(CPORT_NR, &dwCommEvent,NULL);
#endif
		RS232_PollComport(CPORT_NR,get_chk,12);


#ifndef _WIN32
		printf("Connected success! Start uploading %s to COM %d !\n", file_name, CPORT_NR+1);
#else
		set_cur_color(CONSOLE_FG_COLOR_GREEN, 0);
		printf("Connected success! Start uploading ");
		set_cur_color(CONSOLE_FG_COLOR_INST, CONSOLE_BG_COLOR_BLUE);
		printf("%s", file_name);
		set_cur_color(CONSOLE_FG_COLOR_GREEN, 0);
		printf(" to ");
		set_cur_color(CONSOLE_FG_COLOR_INST, CONSOLE_BG_COLOR_BLUE);
		printf("COM %d", CPORT_NR+1);
		set_cur_color(CONSOLE_FG_COLOR_GREEN, 0);
		printf(" !\n\n");
		restore_cur_color();
#endif

		if (!strncpy(get_chk,chk,12)) {
			error = 1;
		}
		if (error) {
			set_cur_color(CONSOLE_FG_COLOR_RED, 0);
			printf("plz press reset bottom, and check mode is program mode\n");
			restore_cur_color();
			return 0;
		}
	}

	int totaltimes = ROUNDUP_QUOT(filesize,45*16);
	int times = 0;
	do {
		print_progressbar((float)times/(float)totaltimes);
		times++;
		parse_hex_data_iter(fp, hex_data, &data_count, &chksum);
		tri_data[5] = data_count/256;
		tri_data[6] = data_count%256;
        tri_data[6+data_count+1] = chksum;
		if(!nouploading) {
			memcpy(tri_data+7,hex_data,data_count);
			RS232_SendBuf(CPORT_NR,tri_data,data_count+8);
			sleep_ms(30);
		}
		if (do_verbose) {
			print_hex_data(hex_data, data_count);
			printf("chksum=%02X\n", chksum);
			printf("--------------------------------------\n");
		}
	} while(data_count==256);
	print_progressbar((float)times/(float)totaltimes);
	if(!nouploading){
		RS232_SendBuf(CPORT_NR,end,8);
#ifdef __WIN32
		RS_WaitCommEvent(CPORT_NR, &dwCommEvent,NULL);
#endif
		RS232_PollComport(CPORT_NR,get_chk,12);
		if (!strncpy(get_chk,chk,12)) {
			error = 1;
		}
	}


	if (error) {
		set_cur_color(CONSOLE_FG_COLOR_RED, 0);
		printf("WORNING : \n");
		set_cur_color(CONSOLE_BG_COLOR_BLUE, 0);
		printf("  Don't press reset bottom in programming!\n");
		printf("  Press the reset bottom and program again.\n");
		restore_cur_color();
		return 0;
	}
	set_cur_color(CONSOLE_FG_COLOR_GREEN, 0);
	printf("\n\nUpload ");
	set_cur_color(CONSOLE_FG_COLOR_INST, CONSOLE_BG_COLOR_BLUE);
	printf("%s", file_name);
	set_cur_color(CONSOLE_FG_COLOR_GREEN, 0);
	printf(" successful !\n");
	restore_cur_color();
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
	set_cur_color(CONSOLE_FG_COLOR_BLUE, 0);
	for (uint16_t j = 0; j < data_count; j++) {
		printf("%02X ",((uint8_t*)data_p)[j]);
		if ((j+1)%16==0) {
			printf("\n");
		}
	}
	restore_cur_color();
}

void print_progressbar(float percentage) {
	int val = (int) (percentage * 100);
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;

	printf("\r [");
	set_cur_color(0, CONSOLE_BG_COLOR_GREEN | CONSOLE_FG_COLOR_INST);
	if(!is_use_color)
		printf("%.*s", lpad, PBSTR);
	else
		printf("%.*s", lpad, PBSTR2);
	set_cur_color(0, CONSOLE_BG_COLOR_BLUE	| CONSOLE_BG_COLOR_INST);
	printf("%*s", rpad, "");
	restore_cur_color();
	printf("]%3d%%", val);
	fflush(stdout);
}

void sleep_ms(unsigned long duration) {
#ifdef __WIN32
	Sleep(duration);
#else
	usleep(1000*duration);
#endif
}

#ifdef _WIN32

void set_cur_color(WORD fg_color, WORD bg_color) {
	if(!is_use_color) return;
	SetConsoleTextAttribute(hConsole, fg_color | bg_color);
}

void restore_cur_color() {
	if(!is_use_color) return;
	SetConsoleTextAttribute(hConsole, saved_attributes);
}

#endif
