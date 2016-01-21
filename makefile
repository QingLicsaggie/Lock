CC := cc
CFLAGS := -c -fPIC -Wall
LDFLAGS := -lrt

HEAD:= lock_manager.h
SOURCE:=lock_manager.c lock_main.c
OBJ   :=$(subst src, ob, $(SOURCE: .c=.o))

all: lock

lock: $(OBJ)
	$(CC)  $(LDFLAGS) -o $@ $^
	
%.o: %.c makefile
	$(CC) $(CFLAGS) -o $@ $< 

clean:
	rm -f *.o lock
