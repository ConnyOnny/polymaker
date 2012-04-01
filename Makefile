polymaker : polymaker.cpp readpng.o
	g++ -O2 polymaker.cpp readpng.o -o polymaker -lpng

readpng.o : readpng.c readpng.h
	gcc -c -O2 -std=c99 readpng.c

clean :
	rm -f *.o polymaker
