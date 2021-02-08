LIBS = -lpthread -lssh
CFLAGS =  -g -ggdb -O0

OBJ = fu.o futex.o ssh_brute.o
SRCS = $(OBJ:%.o=%.c)
HDRS = futex.h ssh_brute.h


fu: $(SRCS)
	gcc -o $@ $(CFLAGS) $(SRCS) $(HDRS) $(LIBS)

clean:
	rm -rf *.o fu
