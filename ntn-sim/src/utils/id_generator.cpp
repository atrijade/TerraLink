#include "ntn-sim/utils/id_generator.h"

namespace NtnSim::Utils {

IdGenerator::IdGenerator() : m_counter(0) {}

IdGenerator::~IdGenerator() {}

uint64_t IdGenerator::GetNextId() {
    return ++m_counter;
}

std::string IdGenerator::GetNextStringId(const std::string& prefix) {
    return prefix + std::to_string(GetNextId());
}

} // namespace NtnSim::Utils
