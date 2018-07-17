//  Created by Julien Aubert on 2018-07-15
//  MIT license
#include "glad/glad.h"
#include "glad/glad_glx.h"

#ifdef __gl_h_
#define USE_OPENGL 1
#define USE_GLES 0
#define USE_GLES2 0
#define USE_GLES3 0
#elif defined(__gl2_h_)
#define USE_OPENGL 0
#define USE_GLES 1
#define USE_GLES2 1
#define USE_GLES3 0
// be more like GLES3
#define GL_SRGB8_ALPHA8 GL_SRGB8_ALPHA8_EXT
#define glGenVertexArrays glGenVertexArraysOES
#define glBindVertexArray glBindVertexArrayOES
#define glDeleteVertexArrays glDeleteVertexArraysOES
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_EXT
// seems GL_EXT_sRGB does not specify values for LINEAR/SRGB that
// would be returned when querying GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING
// assuming here that it is the same as in GL ES 3
#define GL_LINEAR 0x2601
#define GL_SRGB 0x8C40
#elif defined(__gl3_h_)
#define USE_OPENGL 0
#define USE_GLES 1
#define USE_GLES2 0
#define USE_GLES3 1
#endif

#if !USE_OPENGL && !USE_GLES
#error Cannot figure out what OpenGL/GL ES you are running.
#endif

#if USE_OPENGL
#include <GL/gl.h>
#endif
#if USE_GLES
#include <GLES2/gl2.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <dlfcn.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include "gl_compile.h"
#include "gl_error.h"

/*
static void *glx_lib = 0;
static void *glx_GetProcAddress = 0;
static void *glx_GetProcAddressARB = 0;


void *getProcAddressGLX(char const * const name)
{
    if (!glx_lib) {
        glx_lib = dlopen("libGL.so.1", RTLD_GLOBAL);
        if (!glx_lib) glx_lib = dlopen("libGL.so", RTLD_GLOBAL);
        if (glx_lib) glx_GetProcAddress = dlsym(glx_lib, "glXGetProcAddress");
        if (glx_lib) glx_GetProcAddressARB = dlsym(glx_lib, "glXGetProcAddressARB");
    }
    if (!glx_lib) {
        fprintf(stderr, "dlopen failed, tried libGL.so.1 and libGL.so\n");
        exit(1);
    }
    if (glx_GetProcAddress) {
        return glx_GetProcAddress(name);
    }
    if (glx_GetProcAddressARB) {
        return glx_GetProcAddressARB(name);
    }
    return dlsym(glx_lib, name);
}


void *get_proc(char const * const name) {
    void *f = getProcAddressGLX((GLubyte*)name);
    fprintf(stderr, "%s: %p\n", name, f);
    if (!f) exit(1);
    return f;
}*/

struct glx_handles {
    Display *dpy;
    Window win;
    GLXContext glc;
    GLint gl_major;
};

int setup_gl_context(int width, int height, struct glx_handles *out) {
    Display *dpy = XOpenDisplay(0);
    if (!dpy) {
        fprintf(stderr, "XOpenDisplay returned 0\n");
        return 1;
    }
    Window root = DefaultRootWindow(dpy);
    int screen = DefaultScreen(dpy);
    if (!gladLoadGLX(dpy, screen)) {
        fprintf(stderr, "gladLoadGLXLoader failed\n");
        return 1;
    }
    // XXX: get 0 size alpha when specify GLX_RGBA_BIT (i.e. when omitting GLX_ALPHA_SIZE)
    GLint att[] = {
        GLX_DOUBLEBUFFER, True,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_FRAMEBUFFER_SRGB_CAPABLE_EXT, 1,
        GLX_X_VISUAL_TYPE, GLX_DIRECT_COLOR,
        GLX_X_RENDERABLE, True,
        None};
    int num_fbconfigs;
    GLXFBConfig *fbconfigs = glXChooseFBConfig(dpy, screen, att, &num_fbconfigs);
    if (!fbconfigs) {
        fprintf(stderr, "glXChooseFBConfig returned 0\n");
        exit(1);
    }
    GLXFBConfig fbconfig = fbconfigs[0];
    XFree(fbconfigs);
    XVisualInfo *vi = glXGetVisualFromFBConfig(dpy, fbconfig);
    if (!vi) {
        fprintf(stderr, "glXGetVisualFromFBConfig returned 0\n");
        return 1;
    }
    Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    if (!vi) {
        fprintf(stderr, "glXChooseVisual returned 0\n");
        return 1;
    }
    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask;
    Window win = XCreateWindow(dpy, root, 0, 0, width, height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    if (win == BadAlloc ||
        win == BadColor ||
        win == BadCursor ||
        win == BadMatch ||
        win == BadPixmap ||
        win == BadValue ||
        win == BadWindow) {
        fprintf(stderr, "XCreateWindow failed, returned %ld\n", win);
        return 1;
    }
    if (XMapWindow(dpy, win) == BadWindow) {
        fprintf(stderr, "XMapWindow failed\n");
        return 1;
    }
    //GLXContext glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
#if USE_OPENGL
    int const attrib_list[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 6,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB, None};
#elif USE_GLES
    int const attrib_list[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
        GLX_CONTEXT_MINOR_VERSION_ARB, 0,
        GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_ES2_PROFILE_BIT_EXT, None};
#endif
    GLXContext glc = glXCreateContextAttribsARB(dpy, fbconfig, NULL, GL_TRUE, attrib_list);
    if (!glc) {
        fprintf(stderr, "glXCreateContext failed\n");
        return 1;
    }
    if (!glXMakeCurrent(dpy, win, glc)) {
        fprintf(stderr, "glXMakeCurrent failed\n");
        return 1;
    }
#if USE_OPENGL
    if (!gladLoadGLLoader((GLADloadproc)glXGetProcAddress)) {
        fprintf(stderr, "gladLoadGLLoader failed\n");
        return 1;
    }
#elif USE_GLES
    if (!gladLoadGLES2Loader((GLADloadproc)glXGetProcAddress)) {
        fprintf(stderr, "gladLoadGLES2Loader failed\n");
        return 1;
    }
#endif
    const char *version = (char *)glGetString(GL_VERSION);
    if (!version) {
        fprintf(stderr, "glGetString(GL_VERSION) failed\n");
        return 1;
    }
    fprintf(stderr, "glGetString(GL_VERSION): %s\n", version);
    out->dpy = dpy;
    // 10th is always <version number> (and so far 3 or 2 => < 9)
    out->win = win;
    out->glc = glc;
    // assume version < 10
    // OpenGL<space>ES<space><version number><space><vendor-specific information>
    // 10th is always <version number> (and so far 3 or 2 => < 9)
    out->gl_major = version[10] - '0'; 
    if (CHECK_GL()) return 1;
    return 0;
}

struct quadtest {
    GLuint program;
    GLuint position_index;
    GLuint uv_index;
    GLint texture_location;
    GLuint vertex_array;
    GLuint vertex_buffer;
    GLuint texture;
    int vertices_count;
};


GLuint load_srgb8_a8_texture(uint8_t *pixels, int width, int height) {
    GLuint texture = 0;
    glGenTextures(1, &texture); CHECK_GL();
    glBindTexture(GL_TEXTURE_2D, texture); CHECK_GL();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels); CHECK_GL();
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

// assume gl es 2 (version 100) shader, and convert to gl es 3
void fix_shader(char *dst, size_t dst_size, GLint major, char const *src) {
    if (dst_size < strlen(src)+1) {
        fprintf(stderr, "shader source buffer is too small, need at least: %ld\n", strlen(src)+1);
        exit(1);
    }
    memcpy(dst, src, strlen(src)+1);
    if (major == 2) return;
    char *v = strstr(dst, "#version 100");
    if (!v) exit(1);
    v[strlen("#version ")] = '3';
    v[strlen("#version 300 ")] = 'e';
    v[strlen("#version 300 e")] = 's';
    while ((v = strstr(dst, "texture2D"))) {
        memcpy(v+2, v, strlen("texture"));
        v[0] = ' ';
        v[1] = ' ';
    }
    while ((v = strstr(dst, "varying"))) {
        for (int i = 0; i < strlen("varying"); ++i) {
            v[i] = ' ';
        }
    }
    //fprintf(stderr, "pre: \n%s\n", src);
    //fprintf(stderr, "post: \n%s\n", dst);
}

void quadtest_setup(GLint major, struct quadtest *test) {
    char vsh[1000];
    major = 2;
    fix_shader(vsh, 1000, major, ""
        " #version 100 //\n"
        " varying in vec2 position;"
        " in vec2 uv;"
        " varying out lowp vec2 f_uv;"
        " void main() {"
        "     vec4 pos;"
        "     pos.xy = position;"
        "     pos.z = 0.0;"
        "     pos.w = 1.0;"
        "     gl_Position = pos;"
        "     f_uv = uv;"
        " }");
    char fsh[1000];
    fix_shader(fsh, 1000, major, ""
        " #version 100 //\n"
        " uniform lowp sampler2D tex;"
        " in lowp vec2 f_uv;"
        " varying out lowp vec4 fragmentColor;"
        " precision mediump float;"
        " void main() {"
        "     vec4 tx = texture2D(tex, f_uv);" // GLES3: texture
        // NOTE: texture is assumed to be premultiplied alpha
        //"     fragmentColor.rgb = tx.rgb * 8.0;"
        "     fragmentColor.rgb = tx.rgb;"
        "     fragmentColor.a = tx.a;"
        " }");
    GLuint vert_shader = 0;
    GLuint frag_shader = 0;
    if (gl_compile_program_start(vsh, fsh, &test->program, &vert_shader, &frag_shader)) {
        exit(1);
    }
    test->position_index = 0;
    test->uv_index = 1;
    glBindAttribLocation(test->program, test->position_index, "position"); CHECK_GL();
    glBindAttribLocation(test->program, test->uv_index, "uv"); CHECK_GL();
    if (gl_compile_program_finish(test->program, vert_shader, frag_shader)) {
        exit(1);
    }
    CHECK_GL();
    test->texture_location = glGetUniformLocation(test->program, "tex"); CHECK_GL();
    glGenVertexArrays(1, &test->vertex_array); CHECK_GL();
    glBindVertexArray(test->vertex_array); CHECK_GL();
    glGenBuffers(1, &test->vertex_buffer); CHECK_GL();
    glBindBuffer(GL_ARRAY_BUFFER, test->vertex_buffer); CHECK_GL();
    size_t vertex_byte_count = 4 * sizeof (GLfloat);
    size_t vertices_count = 6;
    test->vertices_count = vertices_count;
    GLfloat scale[] = {0.5, 0.5};
    GLfloat p[] = {0, 0};
    // ab
    // cd
    // (acb bcd)
    GLfloat vertices_xy_uv[] = {
         1.f, -1.f,     1.f, 1.f,   // a
        -1.f, -1.f,     0.f, 1.f,   // c
         1.f,  1.f,     1.f, 0.f,   // b
         1.f,  1.f,     1.f, 0.f,   // b
        -1.f, -1.f,     0.f, 1.f,   // c
        -1.f,  1.f,     0.f, 0.f    // d
    };
    for (int i = 0; i < vertices_count; ++i) {
        float x = vertices_xy_uv[i*4+0];
        float y = vertices_xy_uv[i*4+1];
        vertices_xy_uv[i*4+0] = x * scale[0] + p[0];
        vertices_xy_uv[i*4+1] = y * scale[1] + p[1];
    }
    glBufferData(GL_ARRAY_BUFFER, vertices_count * vertex_byte_count, vertices_xy_uv, GL_STATIC_DRAW); CHECK_GL();
    glEnableVertexAttribArray(test->position_index); CHECK_GL();
    glVertexAttribPointer(test->position_index, 2, GL_FLOAT, GL_FALSE, vertex_byte_count, 0); CHECK_GL();
    glEnableVertexAttribArray(test->uv_index); CHECK_GL();
    glVertexAttribPointer(test->uv_index, 2, GL_FLOAT, GL_FALSE, vertex_byte_count, (void *)(2*sizeof (GL_FLOAT))); CHECK_GL();
    // load a texture in sRGB with the lowest value possible (=1) that is 0 in linear
    // if render additive many of these, eg 12, then will be visible
    uint32_t width = 4;
    uint32_t height = 4;
    size_t rowbytes = width*4;
    uint8_t pixels[rowbytes*height];
    for (int py = 0; py < height; ++py) {
        uint8_t *pixel = pixels + py * rowbytes;
        for (int px = 0; px < width; ++px) {
            float cf = 1/255.0;
            pixel[0] = cf*255;
            pixel[1] = cf*255;
            pixel[2] = cf*255;
            pixel[3] = 255;
            pixel += 4;
        }
    }
    test->texture = load_srgb8_a8_texture(pixels, width, height);
}

void quadtest_render(struct quadtest *test) {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glUseProgram(test->program); CHECK_GL();
    glBindVertexArray(test->vertex_array); CHECK_GL();
    glUniform1i(test->texture_location, 0); CHECK_GL();
    glActiveTexture(GL_TEXTURE0); CHECK_GL();
    glBindTexture(GL_TEXTURE_2D, test->texture); CHECK_GL();
    // NOTE: texture must have pre-multiplied alpha
    glBlendFunc(GL_ONE,       GL_ONE); CHECK_GL();
    glBindBuffer(GL_ARRAY_BUFFER, test->vertex_buffer); CHECK_GL();
    for (int i = 0; i < 1; ++i) {
        glDrawArrays(GL_TRIANGLES, 0, test->vertices_count); CHECK_GL();
    }
}

void quadtest_teardown(struct quadtest *test) {
    glDeleteVertexArrays(1, &test->vertex_array);
    glDeleteBuffers(1, &test->vertex_buffer);
    glDeleteProgram(test->program);
    glBindVertexArray(0); CHECK_GL();
    glDeleteTextures(1, &test->texture); CHECK_GL();
}

int main(int argc, char *argv[]) {
    struct glx_handles glx;
    if (setup_gl_context(600, 600, &glx)) return 1;
#if USE_OPENGL
    // note: this does not exist in GL ES
    // (is implicitly always true if gl context has sRGB framebuffer)
    glEnable(GL_FRAMEBUFFER_SRGB);
#endif
    GLint enc = 0;
#if USE_OPENGL
    GLenum default_buffers[] = {GL_FRONT_LEFT, GL_BACK_LEFT, GL_FRONT_RIGHT, GL_BACK_RIGHT};
    char *default_buffers_str[] = {"GL_FRONT_LEFT", "GL_BACK_LEFT", "GL_FRONT_RIGHT", "GL_BACK_RIGHT"};
#else
    GLenum default_buffer = glx.gl_major == 2 ? GL_COLOR_ATTACHMENT0 : GL_BACK;
    char const *default_buffer_str = glx.gl_major == 2 ? "GL_COLOR_ATTACHMENT0" : "GL_BACK";
    GLenum default_buffers[] = {default_buffer};
    char const *default_buffers_str[] = {default_buffer_str};
#endif
    size_t num_default_buffers = sizeof default_buffers / sizeof default_buffers[0];
    for (int i = 0; i < num_default_buffers; ++i) {
        // XXX: odd: this says linear even if it is sRGB
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, default_buffers[i], GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &enc);
        GLint redbits, greenbits, bluebits, alphabits;
#if USE_GLES2
        glGetIntegerv(GL_RED_BITS, &redbits);
        glGetIntegerv(GL_GREEN_BITS, &greenbits);
        glGetIntegerv(GL_BLUE_BITS, &bluebits);
        glGetIntegerv(GL_ALPHA_BITS, &alphabits);
#else
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, default_buffers[i], GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &redbits);
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, default_buffers[i], GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE, &greenbits);
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, default_buffers[i], GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE, &bluebits);
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, default_buffers[i], GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE, &alphabits);
#endif
        fprintf(stderr, "GL_FRAMEBUFFER %s RGBA bits: %d %d %d %d encoding is 0x%x => %s\n", default_buffers_str[i], redbits, greenbits, bluebits, alphabits, enc, enc == GL_SRGB ? "sRGB" : "not sRGB");

    }
    if (CHECK_GL()) return 1;
#if USE_OPENGL
        GLboolean is_srgb;
        glGetBooleanv(GL_FRAMEBUFFER_SRGB, &is_srgb);
        fprintf(stderr, "glGetBooleanv(GL_FRAMEBUFFER_SRGB): %d\n", is_srgb);
#endif
    if (CHECK_GL()) return 1;
    while (1) {
        XEvent xev;
        XNextEvent(glx.dpy, &xev);
        struct quadtest quadtest;
        quadtest_setup(glx.gl_major, &quadtest);
        if(xev.type == Expose) {
            XWindowAttributes gwa;
            XGetWindowAttributes(glx.dpy, glx.win, &gwa);
            glViewport(0, 0, gwa.width, gwa.height);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            quadtest_render(&quadtest); 
            glXSwapBuffers(glx.dpy, glx.win);
        } else if(xev.type == KeyPress) {
            glXMakeCurrent(glx.dpy, None, NULL);
            glXDestroyContext(glx.dpy, glx.glc);
            XDestroyWindow(glx.dpy, glx.win);
            XCloseDisplay(glx.dpy);
            exit(0);
        }
        quadtest_teardown(&quadtest);
    }

    return 0;
}

