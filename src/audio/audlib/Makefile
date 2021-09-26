CC = gcc
CFLAGS = -Wall -O3 -ffast-math -fomit-frame-pointer

ifdef DJDIR
	EXE = .exe
	ALLEGRO = -lalleg
else
	EXE = 
	ALLEGRO = `allegro-config --libs`
endif


all: libaud.a example$(EXE)

%.o : %.c 
	$(CC) $(CFLAGS) -c -o $@ $<

libaud.a : audlib.o
	ar rs $@ $<

example$(EXE) : example.o libaud.a
	$(CC) -o $@ $^ $(ALLEGRO)

clean:
	rm -f example$(EXE) example.o audlib.o libaud.a	
