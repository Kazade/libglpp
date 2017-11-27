#pragma once

#include <kaztest/kaztest.h>
#include "fake_pvr.h"


class BaseTestCase : public TestCase {
public:
    void set_up() {
        TestCase::set_up();
        CallLog::clear();
    }
};
