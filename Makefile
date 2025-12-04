CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -pthread
LDFLAGS = -pthread
TARGETS = master worker client client_bis

all: $(TARGETS)

master: master.c
	$(CC) $(CFLAGS) -o master master.c

worker: worker.c
	$(CC) $(CFLAGS) -o worker worker.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

client_bis: client_bis.c
	$(CC) $(CFLAGS) -o client_bis client_bis.c $(LDFLAGS)

clean:
	rm -f $(TARGETS) *.o
	rm -f tube_client_to_master tube_master_to_client

distclean: clean
	rm -f *~

.PHONY: all clean distclean