#include <cstdint>

typedef struct pvr_vertex {
    uint32_t flags;
    float x;
    float y;
    float z;
    float u;
    float v;
    uint32_t argb;
    uint32_t oargb;
} pvr_vertex_t;


extern FakeFunction<void, pvr_poly_hdr_t*, pvr_poly_cxt_t*> pvr_poly_compile;
extern FakeFunction<void, pvr_poly_cxt_t*, pvr_list_t> pvr_poly_cxt_col;

