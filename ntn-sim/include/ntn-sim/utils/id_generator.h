#pragma once

#include <atomic>
#include <string>

namespace NtnSim::Utils {

/**
 * @brief Thread-safe incremental ID generator.
 */
class IdGenerator {
public:
    IdGenerator();
    ~IdGenerator();

    /**
     * @brief Generates next unique numeric ID.
     */
    uint64_t GetNextId();

    /**
     * @brief Generates next unique string ID with a prefix.
     */
    std::string GetNextStringId(const std::string& prefix);

private:
    std::atomic<uint64_t> m_counter{0};
};

} // namespace NtnSim::Utils
