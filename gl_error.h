//  Created by Julien Aubert on 2016-12-31.
//  MIT license
#ifndef GL_ERROR_H
#define GL_ERROR_H

int _check_gl(char *extra_msg, char *file, int line);
#define CHECK_GL() _check_gl("", __FILE__, __LINE__)

#endif
