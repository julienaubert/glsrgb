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
#include <math.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include "gl_compile.h"
#include "gl_error.h"

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
    GLint ramp_location;
    GLint offset_location;
    GLint scale_location;
    GLuint vertex_array;
    GLuint vertex_buffer;
    int vertices_count;
};

GLuint create_a_texture(uint8_t *pixels, int width, int height) {
    GLuint texture = 0;
    glGenTextures(1, &texture); CHECK_GL();
    glBindTexture(GL_TEXTURE_2D, texture); CHECK_GL();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, pixels); CHECK_GL();
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}

GLuint create_srgb8_a8_texture(uint8_t *pixels, int width, int height) {
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

void quadtest_setup(GLint major, struct quadtest *test, char const *vsh_es2, char const *fsh_es2) {
    char vsh[1000];
    major = 2;
    fix_shader(vsh, 1000, major, vsh_es2);
    char fsh[1000];
    fix_shader(fsh, 1000, major, fsh_es2);
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
    test->ramp_location = glGetUniformLocation(test->program, "ramp"); CHECK_GL();
    test->offset_location = glGetUniformLocation(test->program, "offset"); CHECK_GL();
    test->scale_location = glGetUniformLocation(test->program, "scale"); CHECK_GL();
    glGenVertexArrays(1, &test->vertex_array); CHECK_GL();
    glBindVertexArray(test->vertex_array); CHECK_GL();
    glGenBuffers(1, &test->vertex_buffer); CHECK_GL();
    glBindBuffer(GL_ARRAY_BUFFER, test->vertex_buffer); CHECK_GL();
    size_t vertex_byte_count = 4 * sizeof (GLfloat);
    size_t vertices_count = 6;
    test->vertices_count = vertices_count;
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
    glBufferData(GL_ARRAY_BUFFER, vertices_count * vertex_byte_count, vertices_xy_uv, GL_STATIC_DRAW); CHECK_GL();
    glEnableVertexAttribArray(test->position_index); CHECK_GL();
    glVertexAttribPointer(test->position_index, 2, GL_FLOAT, GL_FALSE, vertex_byte_count, 0); CHECK_GL();
    glEnableVertexAttribArray(test->uv_index); CHECK_GL();
    glVertexAttribPointer(test->uv_index, 2, GL_FLOAT, GL_FALSE, vertex_byte_count, (void *)(2*sizeof (GL_FLOAT))); CHECK_GL();
}

float linear_to_srgb(float linear) {
    // https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_sRGB.txt
    double cl = linear;
    if (isnan(cl)) return 0.0;
    if (cl > 1.0) return 1.0;
    if (cl <= 0.0) return 0.0;
    if (cl < 0.0031308) return 12.92 * cl;
    if (cl < 1) return 1.055 * pow(cl, 0.41666) - 0.055;
    return 1;
}

float srgb_to_linear(float srgb) {
    // see: https://en.wikipedia.org/wiki/SRGB
    // https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_sRGB.txt
    double cs = srgb;
    if (cs < 0) cs = 0;
    if (cs > 1) cs = 1;
    if (cs <= 0.04045) return cs / 12.92;
    return pow((cs + 0.055)/1.055, 2.4);
}

GLuint create_a_texture_srgb_ramp() {
    // 1/255 is smallest value in sRGB format
    // in linear, that value is lmin=1/255/12.92
    // lmin must map to nonzero when we lookup
    // lmin in the srgb ramp
    // so we need 1/lmin = 3066 at minimum.
    // due to rounding/precision, 3277 is minimum
    // make that 4096
    int range = 4096;
    int width = range;
    int height = 1;
    uint8_t pixels[width*height];
    for (int i = 0; i < range; ++i) {
        float cl = (i+0.5)/(float)(range - 1);
        float cs = linear_to_srgb(cl);
        pixels[i] = cs*255.0;
    }
    for (int i = 0; i < range; ++i) {
        fprintf(stderr, "linear: %d srgb: %d back to linear: %d\n", i, pixels[i], (uint8_t)(255.0*srgb_to_linear(pixels[i]/255.0)));
    }
    return create_a_texture(pixels, width, height);
}

GLuint create_srgb8_a8_texture_grey_pma(int width, int height, uint8_t grey, uint8_t alpha) {
    size_t rowbytes = width*4;
    uint8_t pixels[rowbytes*height];
    for (int py = 0; py < height; ++py) {
        uint8_t *pixel = pixels + py * rowbytes;
        for (int px = 0; px < width; ++px) {
            float cf = grey/255.0;
            float a_srgb = linear_to_srgb(alpha);
            pixel[0] = cf*a_srgb*255;
            pixel[1] = cf*a_srgb*255;
            pixel[2] = cf*a_srgb*255;
            pixel[3] = alpha;
            pixel += 4;
        }
    }
    return create_srgb8_a8_texture(pixels, width, height);
}

void quadtest_render(struct quadtest *test, GLuint texture, GLuint ramp, GLfloat offset[2], GLfloat scale[2]) {
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glUseProgram(test->program); CHECK_GL();
    glBindVertexArray(test->vertex_array); CHECK_GL();
    glUniform2f(test->offset_location, offset[0], offset[1]); CHECK_GL();
    glUniform2f(test->scale_location, scale[0], scale[1]); CHECK_GL();
    glUniform1i(test->texture_location, 0); CHECK_GL();
    glUniform1i(test->ramp_location, 1); CHECK_GL();
    glActiveTexture(GL_TEXTURE0); CHECK_GL();
    glBindTexture(GL_TEXTURE_2D, texture); CHECK_GL();
    glActiveTexture(GL_TEXTURE1); CHECK_GL();
    glBindTexture(GL_TEXTURE_2D, ramp); CHECK_GL();
    // NOTE: texture must have pre-multiplied alpha
    glBlendFunc(GL_ONE,       GL_ONE); CHECK_GL();
    glBindBuffer(GL_ARRAY_BUFFER, test->vertex_buffer); CHECK_GL();
    glDrawArrays(GL_TRIANGLES, 0, test->vertices_count); CHECK_GL();
}

void quadtest_teardown(struct quadtest *test) {
    glDeleteVertexArrays(1, &test->vertex_array);
    glDeleteBuffers(1, &test->vertex_buffer);
    glDeleteProgram(test->program);
    glBindVertexArray(0); CHECK_GL();
}

struct fborender {
    GLuint texture;
    GLuint fbo;
    int width, height;
};

int fborender_setup(struct fborender *test, int width, int height) {
    GLuint texture = create_srgb8_a8_texture(0, width, height);
    GLuint fb;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "failed create framebuffer, not complete?\n");
        return 1;
    }
    test->texture = texture;
    test->fbo = fb;
    return 0;
}


void fborender_resize(struct fborender *test, int width, int height) {
    if (test->width == width && test->height == height) return;
    glBindTexture(GL_TEXTURE_2D, test->texture); CHECK_GL();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0); CHECK_GL();
}

void fborender_teardown(struct fborender *test) {
    glDeleteTextures(1, &test->texture);
    glDeleteRenderbuffers(1, &test->fbo);
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
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, default_buffers[i], GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &enc); CHECK_GL();
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
    // load a texture in sRGB with the lowest value possible
    // (i.e. = 1, which is 0 in linear)
    GLuint darkgrey_texture = create_srgb8_a8_texture_grey_pma(4, 4, 1, 255);
    if (!darkgrey_texture) {
        exit(1);
    }
    // for fbo rendering when we don't have sRGB framebuffer, we
    // take linear data to sRGB via a ramp_texture
    GLuint srgb_ramp = create_a_texture_srgb_ramp();
    if (!srgb_ramp) {
        exit(1);
    }
    struct fborender fborender;
    XWindowAttributes gwa;
    XGetWindowAttributes(glx.dpy, glx.win, &gwa);
    if (fborender_setup(&fborender, gwa.width, gwa.height)) {
        exit(1);
    }
    if (CHECK_GL()) return 1;
    int use_fbo = argc > 1;
    struct quadtest quad_darkgrey;
    quadtest_setup(glx.gl_major, &quad_darkgrey,
        " #version 100 //\n"
        " varying in vec2 position;"
        " uniform vec2 offset;"
        " uniform vec2 scale;"
        " in vec2 uv;"
        " varying out lowp vec2 f_uv;"
        " void main() {"
        "     vec4 pos;"
        "     pos.xy = position * scale + offset;"
        "     pos.z = 0.0;"
        "     pos.w = 1.0;"
        "     gl_Position = pos;"
        "     f_uv = uv;"
        " }",
        " #version 100 //\n"
        " uniform lowp sampler2D tex;"
        " in lowp vec2 f_uv;"
        " varying out lowp vec4 fragmentColor;"
        " precision mediump float;"
        " void main() {"
        "     vec4 tx = texture2D(tex, f_uv);" // GLES3: texture
        // NOTE: texture is assumed to be premultiplied alpha
        "     fragmentColor = tx;"
        " }");
    struct quadtest quad_postprocess;
    quadtest_setup(glx.gl_major, &quad_postprocess,
        " #version 100 //\n"
        " varying in vec2 position;"
        " uniform vec2 offset;"
        " uniform vec2 scale;"
        " in vec2 uv;"
        " varying out lowp vec2 f_uv;"
        " void main() {"
        "     vec4 pos;"
        "     pos.xy = position * scale + offset;"
        "     pos.z = 0.0;"
        "     pos.w = 1.0;"
        "     gl_Position = pos;"
        "     f_uv = uv;"
        " }",
        " #version 100 //\n"
        " uniform highp sampler2D tex;"
        " uniform lowp sampler2D ramp;"
        " in lowp vec2 f_uv;"
        " varying out lowp vec4 fragmentColor;"
        " precision mediump float;"
        " void main() {"
        "     vec4 tx = texture2D(tex, f_uv);" // GLES3: texture
        // NOTE: texture is assumed to be premultiplied alpha
        "     vec4 srgb_a = vec4(texture2D(ramp, vec2(tx.r, 0.0)).a,"
        "                        texture2D(ramp, vec2(tx.g, 0.0)).a,"
        "                        texture2D(ramp, vec2(tx.b, 0.0)).a,"
        "                        tx.a);"
        // calculation instead of ramp
        //"     float cs;"
        //"     float cl = tx.r;"
        //"     if (cl < 0.0031308) cs = 12.92 * cl;"
        //"     else cs = 1.055 * pow(cl, 0.41666) - 0.055;"
        //"     srgb_a.rgb = vec3(cs,cs,cs);"
        "     fragmentColor = srgb_a;"
        " }");
    while (1) {
        XEvent xev;
        XNextEvent(glx.dpy, &xev);
        if(xev.type == Expose) {
            XWindowAttributes gwa;
            XGetWindowAttributes(glx.dpy, glx.win, &gwa);
            if (use_fbo) {
                fborender_resize(&fborender, gwa.width, gwa.height);
                glBindFramebuffer(GL_FRAMEBUFFER, fborender.fbo);
            } else {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            fprintf(stderr, "w: %d h:%d\n", gwa.width, gwa.height);
            glViewport(0, 0, gwa.width, gwa.height);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            GLfloat offset[] = {0, 0};
            GLfloat scale[] = {0.5, 0.5};
            quadtest_render(&quad_darkgrey, darkgrey_texture, 0, offset, scale);
            if (use_fbo) {
                GLfloat offset[] = {0, 0};
                GLfloat scale[] = {1, 1};
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                quadtest_render(&quad_postprocess, fborender.texture, srgb_ramp, offset, scale);
            }
            glXSwapBuffers(glx.dpy, glx.win);
        } else if(xev.type == KeyPress) {
            glXMakeCurrent(glx.dpy, None, NULL);
            glXDestroyContext(glx.dpy, glx.glc);
            XDestroyWindow(glx.dpy, glx.win);
            XCloseDisplay(glx.dpy);
            exit(0);
        }
    }
    fborender_teardown(&fborender);
    quadtest_teardown(&quad_darkgrey);
    quadtest_teardown(&quad_postprocess);
    glDeleteTextures(1, &darkgrey_texture); CHECK_GL();
    return 0;
}

