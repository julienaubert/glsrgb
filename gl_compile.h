//  Created by Julien Aubert on 2016-12-31.
//  MIT license
#ifndef GL_UTILS_H
#define GL_UTILS_H

#include "glad/glad.h"

int gl_compile_program_start(const char * const vsh_src, const char * const fsh_src,  GLuint *program, GLuint *vert_shader, GLuint *frag_shader);
int gl_compile_program_finish(GLuint program, GLuint vert_shader, GLuint frag_shader);

#endif

