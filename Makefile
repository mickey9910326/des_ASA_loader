NAME = cmd_ASA_loader

CC = gcc
CFLAGS = -Wall -Wextra

LIBSRC = rs232.c
LIBOBJ = ${LIBSRC:.c=.o}

main :
	$(CC) main.c $(CFLAGS) -L ./D2XX -lftd2xx -o $(NAME)
