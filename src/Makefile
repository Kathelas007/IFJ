#############################################
#	 Projekt:    IFJ 2019
#    Tým:        086
#    Varianta:   II
#    Členové:    Antonín Hubík    (xhubik03)
#                Daša Nosková     (xnosko05)
#                David Holas      (xholas11)
#                Kateřina Mušková (xmusko00)
#    
#    Soubor:     Makefile
#    Autor:      Antonín Hubík    (xhubik03)
#    Úpravy:     	
#############################################

CFLAGS=-std=c99 -Wall -Wextra -DDEBUG
#VFLAGS=--leak-check=full #--show-leak-kinds=all

HEADERF := $(wildcard *.h)
SOURCEF := $(wildcard *.c)
OBJECTF := $(patsubst %.c, %.o, $(SOURCEF))
TSTPATH=../tests/

all: translator

translator: $(SOURCEF) $(HEADERF) $(OBJECTF)
	gcc $(CFLAGS) -o translator $(OBJECTF)

#running the executable using "make testrun"
testrun:
	./translator <input.code >output.code

#running the executable using "make run"
run:
	./translator

#starts automatic testing - DON'T FORGET TO CHECK IF TSTPATH IS CORRECT
test:
	$(TSTPATH)tests.sh
#generating object files for individual modules
%.o: %.c %.h
	gcc $(CFLAGS) -c $< -o $@

clean: 
	rm -f translator $(OBJECTF)

#remove object files, keep executable
cleano:
	rm -f $(OBJECTF)