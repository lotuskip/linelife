# "make" to build normal linelife and llvis
# "make normal" to build only normal linelife
# "make llvis" to build only llvis
# "make noninteractive" to build linelife without Curses interface

#The rule set can be changed here. 'n' is the number of alive neighbours
#the cell in question has. The rules are substituted into an if-statement in
#C++ code ("if(DIERULE)" and "if(BIRTHRULE)").
DIERULE="n < 3 || n > 4"
BIRTHRULE="n == 3"
# Default for Linelife:
#DIERULE="n < 3 || n > 4"
#BIRTHRULE="n == 3"
# Conway's classic rules:
#DIERULE="n < 2 || n > 3"
#BIRTHRULE="n == 3"
# "HighLife mode":
#DIERULE="n < 2 || n > 3"
#BIRTHRULE="n == 3 || n == 6"

#Modify these if necessary:
GCC=clang
GXX=clang++
RM=rm -f
FLAGS=-O2
#FLAGS=-O0 -ggdb -Wall -Wextra
LIBCURSES=-lncurses
LIBPNG=-lpng
##########################################################################

all: normal llvis

normal:
	$(GXX) -o linel linel.cpp -DDIERULE=$(DIERULE) -DBIRTHRULE=$(BIRTHRULE) $(FLAGS) $(LIBCURSES)

noninteractive:
	$(GXX) -o linel linel.cpp -DDIERULE=$(DIERULE) -DBIRTHRULE=$(BIRTHRULE) $(FLAGS) -DNO_INTERFACE

llvis:
	$(GCC) -o llvis llvisual.c $(FLAGS) $(LIBPNG)

clean:
	$(RM) linel llvis

