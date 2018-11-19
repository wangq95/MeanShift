#This file compiles change detection.
CC=g++

INCLUDE:=-I/usr/local/include  
CFLAGS:= -std=c++11 `pkg-config --cflags opencv`

LFLAGS=`pkg-config --libs opencv`

LIBDIR:=-L/usr/local/lib -L/usr/lib/x86_64-linux-gnu -L/usr/lib/
SOURCE=main.cpp

all: $(SOURCE)
	$(CC) $(SOURCE) -o shift $(INCLUDE) $(CFLAGS) $(LFLAGS) $(LIBDIR)

clean:
	rm *.o  

