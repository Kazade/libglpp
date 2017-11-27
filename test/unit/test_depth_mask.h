#pragma once

namespace {

class TestDepthMask : public BaseTestCase {
public:
    void test_depth_mask_toggle() {
        glDepthMask(GL_FALSE);

        draw_fullscreen_quad();

        glDepthMask(GL_TRUE);

        draw_quad(1.0f);

        assert_equal(
            CallLog::call(0),
            CallLog::entry("pvr_list_begin", 1, 2)
        );
    }
};

}
