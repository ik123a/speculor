#include "speculor/concurrency/ring_buffer.hpp"
#include "speculor/core/event.hpp"
#include <cstring>

namespace speculor {

// Explicit instantiation for Event type
template class RingBuffer<Event>;

} // namespace speculor