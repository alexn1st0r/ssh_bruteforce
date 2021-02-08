fu: fu.c
	gcc fu.c -o fu -lpthread -lssh -g -ggdb -O0

clean:
	rm -rf *.o fu
