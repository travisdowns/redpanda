

#include "test_utils/global_test_hooks.h"

#include <boost/test/unit_test_suite.hpp>

using namespace boost::unit_test;

class boost_hooks : public global_configuration {
public:
    virtual void test_unit_start(const test_unit& test) override {
        // fmt::print(stdout, "TEST UNIT START: {}\n", test.full_name());
        test_hooks::before_test_case(test.full_name());
    }
};

BOOST_TEST_GLOBAL_CONFIGURATION(boost_hooks);
