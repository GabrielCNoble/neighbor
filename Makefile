CC = g++
CFLAGS = -std=c++11 -Wno-write-strings

DS = dstuff
DSTUFF = $(wildcard $(DS)/accel_structs/*.c) \
		 $(wildcard $(DS)/containers/*.c) \
		 $(wildcard $(DS)/file/*.c) \
		 $(wildcard $(DS)/loaders/*.c) \
		 $(wildcard $(DS)/math/*.c) \
		 $(wildcard $(DS)/physics/*.c)

SRC = main.c mdl.c r_main.c r_vk.c in.c g.c ent.c $(DSTUFF)
OBJ = $(SRC:.c=.o)

$(OBJ) : $(SRC)
	$(CC) -c $< $(CFLAGS) -o $@

all: $(OBJ)
	$(CC) $^ -o main.exe 

clean:
	del $(OBJ)




