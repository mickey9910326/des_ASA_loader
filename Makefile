TARGET = cmd_ASA_loader

CC = gcc
CFLAGS = -Wall -Wextra

LIBSRC = rs232.c
LIBOBJ = ${LIBSRC:.c=.o}

REMOVE = rm -f

main :
	@$(CC) main.c $(LIBSRC) $(CFLAGS) -o $(TARGET)

clean : clean_list

clean_list :
	@echo
	@$(REMOVE) $(TARGET)
