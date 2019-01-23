# Makefile for Debian GNU/Linux: gcc 3.3.3, glew 1.2.2, freeglut 2.2

BASE = helloGPGPU_GLSL
EXE = ${BASE}_linux

all: ${EXE}

${EXE}: ${BASE}.o
	${LINK.cpp} -s -o $@ $< -lGLEW -lglut -lGL

${BASE}.o: ${BASE}.cpp

clean:
	-rm -f *.o

clobber: clean
	-rm -f ${EXE}

