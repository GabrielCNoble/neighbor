CC = g++
CFLAGS = -std=c++11 -Wno-write-strings


DSTUFF_DIRS = $(wildcard dstuff/*)
DSTUFF = $(wildcard $(DSTUFF_DIRS:=/*.c))

SRC = $(wildcard *.c) $(DSTUFF)
LIBINC = -L"vulkan/lib" -L"SDL/lib"
LIB = $(LIBINC) -lSDL2 -lvulkan-1
OBJ = $(SRC:.c=.o)

%.o : %.c
	$(CC) -c $< $(CFLAGS) -o $@

all: $(OBJ)
	$(CC) $^ $(LIB) -o main.exe 

clean:
	del $(subst /,\,$(OBJ))




