// Copyright 2020 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#define BOOST_TEST_MODULE random_seeding

#include "random/fast_prng.h"
#include "random/generators.h"

#include <boost/test/unit_test.hpp>

#include <set>

using random_generators::get_int;
using random_generators::random_state;

// Any object of random_state encapsulates RNG state. I.e., two random_state
// objects with the same state will generate the same sequence of random
// numbers.
//
// The global() object is a thead-local shared instance, used to migrate
// existing callers of the free-function interface, i.e.,
// `random_generators::get_int()` becomes
// `random_generators::global().get_int()`, but new use should consider creating
// and maintaining their own random_state object.

constexpr int fixed_seed = 0xBADF00D;

BOOST_AUTO_TEST_CASE(test_expected_values) {
    random_state rng{fixed_seed};
    BOOST_CHECK_EQUAL(rng.get_int<int>(), 880186458);
}

BOOST_AUTO_TEST_CASE(test_expected_values2) {
    random_state rng{fixed_seed};
    BOOST_CHECK_EQUAL(rng.get_int<int>(), 880186458);
}
