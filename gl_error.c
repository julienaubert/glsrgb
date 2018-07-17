//  Created by Julien Aubert on 2016-12-31.
//  MIT license
#include <stdio.h>
#include "gl_error.h"
#include "glad/glad.h"

int _check_gl(char *extra_msg, char *file, int line) {
    GLenum err = glGetError();
    const char s_GL_INVALID_ENUM[] = "GL_INVALID_ENUM";
    const char i_GL_INVALID_ENUM[] = "An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.";
    const char s_GL_INVALID_VALUE[] = "GL_INVALID_VALUE";
    const char i_GL_INVALID_VALUE[] = "A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.";
    const char s_GL_INVALID_OPERATION[] = "GL_INVALID_OPERATION";
    const char i_GL_INVALID_OPERATION[] = "The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.";
    const char s_GL_INVALID_FRAMEBUFFER_OPERATION[] = "GL_INVALID_FRAMEBUFFER_OPERATION";
    const char i_GL_INVALID_FRAMEBUFFER_OPERATION[] = "The command is trying to render to or read from the framebuffer while the currently bound framebuffer is not framebuffer complete (i.e. the return value from glCheckFramebufferStatus is not GL_FRAMEBUFFER_COMPLETE). The offending command is ignored and has no other side effect than to set the error flag.";
    const char s_GL_OUT_OF_MEMORY[] = "GL_OUT_OF_MEMORY";
    const char i_GL_OUT_OF_MEMORY[] = "There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.";
    const char *info_msg = 0;
    const char *short_msg = 0;
    switch (err) {
        case GL_INVALID_ENUM:
            info_msg = i_GL_INVALID_ENUM;
            short_msg = s_GL_INVALID_ENUM;
            break;
        case GL_INVALID_VALUE:
            info_msg = i_GL_INVALID_VALUE;
            short_msg = s_GL_INVALID_VALUE;
            break;
        case GL_INVALID_OPERATION:
            info_msg = i_GL_INVALID_OPERATION;
            short_msg = s_GL_INVALID_OPERATION;
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            info_msg = i_GL_INVALID_FRAMEBUFFER_OPERATION;
            short_msg = s_GL_INVALID_FRAMEBUFFER_OPERATION;
            break;
        case GL_OUT_OF_MEMORY:
            info_msg = i_GL_OUT_OF_MEMORY;
            short_msg = s_GL_OUT_OF_MEMORY;
            break;
        default:
            info_msg = "UNKNOWN GL ERROR CODE";
            short_msg = "UNKNOWN GL ERROR CODE";
            break;
    }
    if (err != GL_NO_ERROR) {
        fprintf(stderr, "%s %s at %s:%d GL error code: 0x%x GL error info %s\n", extra_msg, short_msg, file, line, err, info_msg);
        return 1;
    }
    return 0;
}

