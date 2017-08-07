
CC = gcc

CFLAGS = -Wall -gdwarf-2 -std=gnu99 -o2

LIBSRC = rs232.c
LIBOBJ = ${LIBSRC:.c=.o}

main :
	@gcc main.c rs232.c -Wall -Wextra -o des_ASA_loader
