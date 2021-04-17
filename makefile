graphictest: driverutils.o graphictest.o framebuffer.o
	gcc -g -o graphictest driverutils.o graphictest.o framebuffer.o

driverutils.o: driverutils.c
	gcc -g -c driverutils.c -o driverutils.o

graphictest.o: graphictest.c
	gcc -g -c graphictest.c -o graphictest.o
	
framebuffer.o: framebuffer.c
	gcc -g -c framebuffer.c -o framebuffer.o
