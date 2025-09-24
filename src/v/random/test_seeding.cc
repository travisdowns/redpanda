

#include "random/test_seeding.h"

#include "random/generators.h"

namespace random_generators {
// Reset the global seed for the random_generators::global() object, so
// the default for testing, so that unit tests see the same series of random
// numbers.
void reset_seed_for_tests() { internal::increment_seed_generation(); }

} // namespace random_generators
