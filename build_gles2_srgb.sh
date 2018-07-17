#!/usr/bin/env sh
glad=glad-es2+srgb
glad_glx=glad-glx-1.4
gcc -I ${glad}/include -I ${glad_glx}/include -Wall -g -c -o ${glad}/src/glad.o ${glad}/src/glad.c
gcc -I ${glad}/include -I ${glad_glx}/include -Wall -g -c -o ${glad_glx}/src/glad_glx.o ${glad_glx}/src/glad_glx.c
gcc -I ${glad}/include -I ${glad_glx}/include -Wall -pedantic -g -o gles_srgb ${glad}/src/glad.o ${glad_glx}/src/glad_glx.o main.c gl_error.c gl_compile.c -lX11 -lGL -lGLU -ldl -lm
