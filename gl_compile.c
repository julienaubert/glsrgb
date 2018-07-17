//  Created by Julien Aubert on 2016-12-31.
//  MIT license
#include <stdlib.h>
#include <stdio.h>
#include "glad/glad.h"
#include "gl_compile.h"
#include "gl_error.h"

int _print_gl_shader_log(GLuint shader)
{
    GLint log_len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
    if (log_len > 0) {
        char *log = (char *)malloc(sizeof(char)*log_len);
        if (!log) {
            fprintf(stderr, "out of mem\n");
            return 1;
        }
        glGetShaderInfoLog(shader, log_len, 0, log);
        fprintf(stderr, "Shader compile log:\n%s", log);
        free(log);
    }
    return 1;
}

int _print_gl_program_log(GLuint prog)
{
    GLint log_len = 0;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);
    if (log_len > 0) {
        char *log = (char *)malloc(sizeof(char)*(log_len));
        if (!log) {
            fprintf(stderr, "out of mem\n");
            return 1;
        }
        glGetProgramInfoLog(prog, log_len, 0, log);
        fprintf(stderr, "Program log:\n%s", log);
        free(log);
    }
    return 0;
}

int _compile_shader(GLuint *shader, GLenum type, const char *src) {
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &src, 0);
    glCompileShader(*shader);
    GLint status = 0;
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        _print_gl_shader_log(*shader);
        glDeleteShader(*shader);
        return 1;
    }
    _print_gl_shader_log(*shader);
    return 0;
}

int _link_program(GLuint prog) {
    GLint status = 0;
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0) {
        _print_gl_program_log(prog);
        return 1;
    }
    _print_gl_program_log(prog);
    return 0;
}

int _validate_program(GLuint prog) {
    glValidateProgram(prog);
    GLint status = 0;
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == 0) {
        _print_gl_program_log(prog);
        return 1;
    }
    _print_gl_program_log(prog);
    return 0;
}

int gl_compile_program_start(const char * const vsh_src, const char * const fsh_src,  GLuint *program, GLuint *vert_shader, GLuint *frag_shader) {
    *vert_shader = 0;
    *frag_shader = 0;
    *program = 0;
    if (_compile_shader(vert_shader, GL_VERTEX_SHADER, vsh_src)) {
        fprintf(stderr, "Failed to compile vertex shader:\n%s\n", vsh_src);
        return 1;
    }
    fprintf(stderr, "compiled vertex shader: \n%s\n", vsh_src);
    if (_compile_shader(frag_shader, GL_FRAGMENT_SHADER, fsh_src)) {
        fprintf(stderr, "Failed to compile fragment shader:\n%s\n", fsh_src);
        glDeleteShader(*vert_shader);
        return 1;
    }
    fprintf(stderr, "compiled fragment shader: \n%s\n", fsh_src);
    *program = glCreateProgram();
    glAttachShader(*program, *vert_shader);
    if (CHECK_GL()) return 1;
    glAttachShader(*program, *frag_shader);
    if (CHECK_GL()) return 1;
    return 0;
}

int gl_compile_program_finish(GLuint program, GLuint vert_shader, GLuint frag_shader) {
    if (_link_program(program)) {
        fprintf(stderr, "Failed to link program: %d\n", program);
        glDeleteShader(vert_shader);
        glDeleteShader(frag_shader);
        return 1;
    }
    glDetachShader(program, vert_shader);
    glDeleteShader(vert_shader);
    glDetachShader(program, frag_shader);
    glDeleteShader(frag_shader);
    return 0;
}

