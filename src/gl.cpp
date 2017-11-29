#include <vector>
#include <cstring>
#include <dc/pvr.h>
#include <cstdio>

#include "private.h"

#define GL_PVR_VERTEX_BUF_SIZE  2560 * 256

#define mat_trans_fv12() { \
        __asm__ __volatile__( \
                              "fldi1 fr15\n" \
                              "ftrv	 xmtrx, fv12\n" \
                              "fldi1 fr14\n" \
                              "fdiv	 fr15, fr14\n" \
                              "fmul	 fr14, fr12\n" \
                              "fmul	 fr14, fr13\n" \
                              : "=f" (__x), "=f" (__y), "=f" (__z) \
                              : "0" (__x), "1" (__y), "2" (__z) \
                              : "fr15" ); \
    }


static AttributePointer VERTEX_POINTER;
static AttributePointer TEXCOORD0_POINTER;
static AttributePointer TEXCOORD1_POINTER;
static AttributePointer NORMAL_POINTER;
static AttributePointer COLOR_POINTER;

static GLboolean POINT_SPRITES_ENABLED = GL_FALSE;
static GLubyte ACTIVE_TEXTURE = 0;
static GLubyte ACTIVE_CLIENT_TEXTURE = 0;
static GLenum ENABLED_CLIENT_STATE = 0;
static GLenum ERROR = GL_NO_ERROR;

static AlignedStack<Mat4, MAX_STACK_DEPTH> MODELVIEW_STACK;
static AlignedStack<Mat4, MAX_STACK_DEPTH> PROJECTION_STACK;
static AlignedStack<Mat4, MAX_STACK_DEPTH> TEXTURE_STACK;
AlignedStack<Mat4, MAX_STACK_DEPTH>* CURRENT_STACK = &MODELVIEW_STACK;

static void _load_identity(Mat4& out) {
    static const float IDENTITY[4][4] = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };

    std::memcpy(&out.m, IDENTITY, sizeof(float) * 16);
}

static Mat4 VIEWPORT_TRANSFORM;

/* Concatenation of the MODELVIEW, PROJECTION and VIEWPORT_TRANSFORM matrices */
static Mat4 RENDER_TRANSFORM;

static std::vector<PVRCommand> OPAQUE_LIST;
static std::vector<PVRCommand> TRANSPARENT_LIST;
static std::vector<PVRCommand> MULTITEXTURE_LIST;
static std::vector<PVRCommand>* CURRENT_LIST = &OPAQUE_LIST;

/* Immediate mode emulation */
static GLboolean IN_IMMEDIATE_MODE_BLOCK = false;
static GLenum IMMEDIATE_MODE_POLYGON_TYPE = GL_TRIANGLES;
static std::vector<float> IMMEDIATE_MODE_VERTEX_DATA;

static struct Viewport {
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
} VIEWPORT;

static struct ContextState {
    float clear_colour [3] = {0.5f, 0.5f, 0.5f};
} CONTEXT_STATE;

static void _set_error(GLenum error) {
    if(ERROR != GL_NO_ERROR) {
        // Only set the first erro
        return;
    }

    ERROR = error;
}

static bool _check_size(GLint size) {
    if(size < 2 || size > 4) {
        _set_error(GL_INVALID_VALUE);
        return false;
    }

    return true;
}

static bool _check_type(GLenum type) {
    if(type != GL_SHORT && type != GL_INT && type != GL_FLOAT && type != GL_DOUBLE) {
        return false;
    }

    return true;
}

static bool _check_mode(GLenum mode) {
    switch(mode) {
    case GL_TRIANGLES:
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLE_STRIP:
        return true;
    case GL_POINTS:
        /* Points only valid if the point sprite extension is enabled */
        return POINT_SPRITES_ENABLED;
    default:
        return false;
    }
}

static bool _multitexture_coords_enabled() {
    return (ENABLED_CLIENT_STATE & ATTRIBUTE_ARRAY_TEXCOORD1) == ATTRIBUTE_ARRAY_TEXCOORD1;
}

static void _prepare_render_matrix() {
    mat_load(&VIEWPORT_TRANSFORM.m);
    mat_apply(&PROJECTION_STACK.top().m);
    mat_apply(&MODELVIEW_STACK.top().m);
    mat_store(&RENDER_TRANSFORM.m);
}

void APIENTRY glInitContextDC() {
    ENABLED_CLIENT_STATE = 0;
    ERROR = GL_NO_ERROR;
    ENABLED_CLIENT_STATE = 0;
    POINT_SPRITES_ENABLED = GL_FALSE;

    Mat4 identity;

    _load_identity(identity);
    _load_identity(VIEWPORT_TRANSFORM);
    _load_identity(RENDER_TRANSFORM);

    MODELVIEW_STACK.push(identity);
    PROJECTION_STACK.push(identity);
    TEXTURE_STACK.push(identity);

    /* Reserve 32k of space in each list initially */
    OPAQUE_LIST.reserve(1024);
    TRANSPARENT_LIST.reserve(1024);
    MULTITEXTURE_LIST.reserve(1024);

    pvr_init_params_t params = {
        /* Enable opaque and translucent polygons with size 32 and 32 */
        { PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_0 },
        GL_PVR_VERTEX_BUF_SIZE, /* Vertex buffer size */
        0, /* No DMA */
        0, /* No FSAA */
        1 /* Disable translucent auto-sorting to match traditional GL */
    };

    pvr_init(&params);
}

static inline void _submit_list_to_pvr(const std::vector<PVRCommand>& list) {
    uint32_t *d = TA_SQ_ADDR;
    uint32_t *s = (uint32_t*) &list[0];
    uint32_t n = list.size();

    /* fill/write queues as many times necessary */
    while(n--) {
        asm("pref @%0" : : "r"(s + 8));  /* prefetch 32 bytes for next loop */
        d[0] = *(s++);
        d[1] = *(s++);
        d[2] = *(s++);
        d[3] = *(s++);
        d[4] = *(s++);
        d[5] = *(s++);
        d[6] = *(s++);
        d[7] = *(s++);
        asm("pref @%0" : : "r"(d));
        d += 8;
    }

    /* Wait for both store queues to complete */
    d = (GLuint *)0xe0000000;
    d[0] = d[8] = 0;
}

void _push_poly_header() {
    pvr_poly_hdr_t hdr;
    pvr_poly_cxt_t cxt;

    cxt.gen.culling = PVR_CULLING_CW;

    cxt.depth.comparison = PVR_DEPTHCMP_GEQUAL;
    cxt.depth.write = PVR_DEPTHWRITE_ENABLE;

    pvr_poly_cxt_col(
        &cxt,
        (CURRENT_LIST == &OPAQUE_LIST) ? PVR_LIST_OP_POLY : PVR_LIST_TR_POLY
    );

    pvr_poly_compile(&hdr, &cxt);

    CURRENT_LIST->push_back(
        reinterpret_cast<const PVRCommand&>(hdr)
    );
}

void _push_multitexture_header() {

}

void APIENTRY glSwapBuffersDC() {
    //printf("Submitting %d\n", OPAQUE_LIST.size());
    pvr_scene_begin();

    pvr_list_begin(PVR_LIST_OP_POLY);
        _submit_list_to_pvr(OPAQUE_LIST);
    pvr_list_finish();
/*
    pvr_list_begin(PVR_LIST_TR_POLY);
        _submit_list_to_pvr(TRANSPARENT_LIST);
        _submit_list_to_pvr(MULTITEXTURE_LIST);
    pvr_list_finish();*/

    pvr_scene_finish();

    OPAQUE_LIST.resize(0);
    TRANSPARENT_LIST.resize(0);
    MULTITEXTURE_LIST.resize(0);
}

void APIENTRY glClear(GLuint mode) {
    if(mode & GL_COLOR_BUFFER_BIT) {
        pvr_set_bg_color(CONTEXT_STATE.clear_colour[0], CONTEXT_STATE.clear_colour[1], CONTEXT_STATE.clear_colour[2]);
    }
}

void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    VIEWPORT.x = x;
    VIEWPORT.y = y;
    VIEWPORT.width = width;
    VIEWPORT.height = height;

    auto& vt = VIEWPORT_TRANSFORM.m;

    vt[0][0] = GLfloat(width) * 0.5f;
    vt[1][1] = -(GLfloat(height) * 0.5f);
    vt[2][2] = 1.0f;

    vt[3][0] = vt[0][0] + GLfloat(x);
    vt[3][1] = vid_mode->height - (vt[1][1] + GLfloat(x));
}

void APIENTRY glMatrixMode(GLenum mode) {
    switch(mode) {
        case GL_MODELVIEW:
            CURRENT_STACK = &MODELVIEW_STACK;
        break;
        case GL_PROJECTION:
            CURRENT_STACK = &PROJECTION_STACK;
        break;
        case GL_TEXTURE:
            CURRENT_STACK = &TEXTURE_STACK;
        break;
        default:
            _set_error(GL_INVALID_ENUM);
    }
}

void APIENTRY glLoadIdentity() {
    _load_identity(CURRENT_STACK->top());
}

void APIENTRY glLoadMatrixf(const GLfloat *m) {
    auto& ml = CURRENT_STACK->top();
    std::memcpy(&ml, m, sizeof(Mat4));
}

void APIENTRY glLoadTransposeMatrixf(const GLfloat *m) {
    auto& ml = CURRENT_STACK->top().m;

    ml[0][0] = m[0];
    ml[0][1] = m[4];
    ml[0][2] = m[8];
    ml[0][3] = m[12];
    ml[1][0] = m[1];
    ml[1][1] = m[5];
    ml[1][2] = m[9];
    ml[1][3] = m[13];
    ml[2][0] = m[2];
    ml[2][1] = m[6];
    ml[2][2] = m[10];
    ml[2][3] = m[14];
    ml[3][0] = m[3];
    ml[3][1] = m[7];
    ml[3][2] = m[11];
    ml[3][3] = m[15];
}

void APIENTRY glMultMatrixf(const GLfloat *m) {
    mat_load(&CURRENT_STACK->top().m);
    mat_apply((GLfloat(*)[4][4]) m);
    mat_store(&CURRENT_STACK->top().m);
}

void APIENTRY glMultTransposeMatrixf(const GLfloat *m) {

}

void APIENTRY glPushMatrix() {
    if(IN_IMMEDIATE_MODE_BLOCK) {
        _set_error(GL_INVALID_OPERATION);
        return;
    }

    if(!CURRENT_STACK->push(CURRENT_STACK->top())) {
        _set_error(GL_STACK_OVERFLOW);
    }
}

void APIENTRY glPopMatrix() {
    if(IN_IMMEDIATE_MODE_BLOCK) {
        _set_error(GL_INVALID_OPERATION);
        return;
    }

    if(!CURRENT_STACK->pop()) {
        _set_error(GL_STACK_UNDERFLOW);
    }
}

void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    auto& m = CURRENT_STACK->top().m;

    m[3][0] += x;
    m[3][1] += y;
    m[3][2] += z;
}

void APIENTRY glActiveTexture(GLenum texture) {
    ACTIVE_TEXTURE = (texture == GL_TEXTURE0) ? 0 : 1;
}

void APIENTRY glEnable(GLenum cap) {
    switch(cap) {
    case GL_POINT_SPRITE_ARB: POINT_SPRITES_ENABLED = GL_TRUE; break;
    default:
        _set_error(GL_INVALID_ENUM);
    }
}

void APIENTRY glDisable(GLenum cap) {
    switch(cap) {
    case GL_POINT_SPRITE_ARB: POINT_SPRITES_ENABLED = GL_FALSE; break;
    default:
        _set_error(GL_INVALID_ENUM);
    }
}

void APIENTRY glEnableClientState(GLenum cap) {
    switch(cap) {
        case GL_VERTEX_ARRAY: ENABLED_CLIENT_STATE |= ATTRIBUTE_ARRAY_VERTEX; break;
        case GL_COLOR_ARRAY: ENABLED_CLIENT_STATE |= ATTRIBUTE_ARRAY_COLOR; break;
        case GL_NORMAL_ARRAY: ENABLED_CLIENT_STATE |= ATTRIBUTE_ARRAY_NORMAL; break;
        case GL_TEXTURE_COORD_ARRAY:
            ENABLED_CLIENT_STATE |= (ACTIVE_CLIENT_TEXTURE == 0) ? ATTRIBUTE_ARRAY_TEXCOORD0 : ATTRIBUTE_ARRAY_TEXCOORD1;
        break;
    default:
        _set_error(GL_INVALID_ENUM);
    }
}

void APIENTRY glDisableClientState(GLenum cap) {
    switch(cap) {
        case GL_VERTEX_ARRAY: ENABLED_CLIENT_STATE &= ~ATTRIBUTE_ARRAY_VERTEX; break;
        case GL_COLOR_ARRAY: ENABLED_CLIENT_STATE &= ~ATTRIBUTE_ARRAY_COLOR; break;
        case GL_NORMAL_ARRAY: ENABLED_CLIENT_STATE &= ~ATTRIBUTE_ARRAY_NORMAL; break;
        case GL_TEXTURE_COORD_ARRAY:
            ENABLED_CLIENT_STATE &= ~((ACTIVE_CLIENT_TEXTURE == 0) ? ATTRIBUTE_ARRAY_TEXCOORD0 : ATTRIBUTE_ARRAY_TEXCOORD1);
        break;
    default:
        _set_error(GL_INVALID_ENUM);
    }
}

void APIENTRY glClientActiveTexture(GLenum texture) {
    ACTIVE_CLIENT_TEXTURE = (texture == GL_TEXTURE0) ? 0 : 1;
}

void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    if(!_check_size(size) || !_check_type(type)) {
        return;
    }

    if(stride < 0) {
        _set_error(GL_INVALID_VALUE);
        return;
    }

    VERTEX_POINTER.size = size;
    VERTEX_POINTER.type = type;
    VERTEX_POINTER.stride = (stride == 0) ? size * sizeof(type) : stride;
    VERTEX_POINTER.pointer = (uint8_t*) pointer;
}

void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    if(!_check_size(size) || !_check_type(type)) {
        return;
    }

    if(stride < 0) {
        _set_error(GL_INVALID_VALUE);
        return;
    }

    auto POINTER = (ACTIVE_CLIENT_TEXTURE == 0) ? &TEXCOORD0_POINTER : &TEXCOORD1_POINTER;

    POINTER->size = size;
    POINTER->type = type;
    POINTER->stride = (stride == 0) ? size * sizeof(type) : stride;
    POINTER->pointer = (uint8_t*) pointer;
}

constexpr uint32_t type_size(GLenum type) {
    return (type == GL_SHORT) ? sizeof(GLshort) :
            (type == GL_INT) ? sizeof(GLint) :
            (type == GL_FLOAT) ? sizeof(GLfloat) :
            (type == GL_DOUBLE) ? sizeof(GLdouble) : 0;
}

template<typename T>
static void handle_vertex(pvr_vertex_t& vout, uint32_t i) {
    const auto offset = i * VERTEX_POINTER.stride;
    T* ptr = (T*) (VERTEX_POINTER.pointer + offset);

    float x = (float) *(ptr++);
    float y = (float) *(ptr++);
    float z = (float) (VERTEX_POINTER.size > 2) ? *(ptr++) : 0;

    auto& mv = RENDER_TRANSFORM.m;

    vout.x = x * mv[0][0] + y * mv[1][0] + z * mv[2][0] + mv[3][0];
    vout.y = x * mv[0][1] + y * mv[1][1] + z * mv[2][1] + mv[3][1];
    vout.z = x * mv[0][2] + y * mv[1][2] + z * mv[2][2] + mv[3][2];
}

template<typename T>
static void handle_texcoord(pvr_vertex_t& vout, uint32_t i, int8_t texture_unit) {
    switch(texture_unit) {
        case 0: {
            const auto toffset = i * TEXCOORD0_POINTER.stride;
            const auto tts = type_size(TEXCOORD0_POINTER.type);
            vout.u = TEXCOORD0_POINTER.pointer[toffset + 0];
            vout.v = TEXCOORD0_POINTER.pointer[toffset + tts];
        } break;
        case 1: {
            const auto toffset = i * TEXCOORD1_POINTER.stride;
            const auto tts = type_size(TEXCOORD1_POINTER.type);
            vout.u = TEXCOORD1_POINTER.pointer[toffset + 0];
            vout.v = TEXCOORD1_POINTER.pointer[toffset + tts];
        } break;
    default:
        vout.u = vout.v = 0;
    }
}

static void vertex_noop(pvr_vertex_t&, uint32_t) {}

GLAPI void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if(!_check_mode(mode)) {
        return;
    }

    _push_poly_header();

    if(_multitexture_coords_enabled()) {
        _push_multitexture_header();
    }

    void (*vertex_func) (pvr_vertex_t&, uint32_t) = nullptr;

    switch(VERTEX_POINTER.type) {
        case GL_DOUBLE:
        case GL_FLOAT: vertex_func = handle_vertex<GLfloat>; break;
        case GL_INT: vertex_func = handle_vertex<GLint>; break;
        case GL_SHORT: vertex_func = handle_vertex<GLshort>; break;
    default:
        vertex_func = vertex_noop;
    }

    /* Concatenate all the matrices for the final vertex -> screenspace transform */
    _prepare_render_matrix();

    for(auto i = first; i < first + count; ++i) {
        pvr_vertex_t vert;
        vertex_func(vert, i);

        CURRENT_LIST->push_back(reinterpret_cast<const PVRCommand&>(vert));
        /*
        if(_multitexture_coords_enabled()) {
            handle_vertex(vert, i, 1);
            MULTITEXTURE_LIST.push_back(reinterpret_cast<const PVRCommand&>(vert));
        }*/
    }
}

GLAPI void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    if(!_check_mode(mode)) {
        return;
    }

    /*_push_poly_header();

    auto iterator = IndexedIterator(mode, count, type, indices, GL_TEXTURE0);
    do {
        CURRENT_LIST->push_back((PVRCommand) (*iterator));
    } while(iterator++); */
}


/* IMMEDIATE MODE EMULATION BELOW */
void APIENTRY glBegin(GLenum mode) {
    if(IN_IMMEDIATE_MODE_BLOCK) {
        _set_error(GL_INVALID_OPERATION);
        return;
    }

    IN_IMMEDIATE_MODE_BLOCK = true;
    IMMEDIATE_MODE_POLYGON_TYPE = mode;
    IMMEDIATE_MODE_VERTEX_DATA.clear();
}

void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    IMMEDIATE_MODE_VERTEX_DATA.push_back(x);
    IMMEDIATE_MODE_VERTEX_DATA.push_back(y);
    IMMEDIATE_MODE_VERTEX_DATA.push_back(z);
}

void APIENTRY glEnd() {
    if(!IN_IMMEDIATE_MODE_BLOCK) {
        _set_error(GL_INVALID_OPERATION);
    }

    IN_IMMEDIATE_MODE_BLOCK = false;

    auto stored_state = ENABLED_CLIENT_STATE;
    auto stored_vertex_pointer = VERTEX_POINTER;

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, &IMMEDIATE_MODE_VERTEX_DATA[0]);
    glDrawArrays(IMMEDIATE_MODE_POLYGON_TYPE, 0, IMMEDIATE_MODE_VERTEX_DATA.size() / 3);

    ENABLED_CLIENT_STATE = stored_state;
    VERTEX_POINTER = stored_vertex_pointer;
}


