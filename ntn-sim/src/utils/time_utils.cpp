#include "ntn-sim/utils/time_utils.h"

namespace NtnSim::Utils {

std::string TimeUtils::FormatSimTime(double simSeconds) {
    (void)simSeconds;
    // Skeleton implementation
    return "00:00:00.000";
}

std::string TimeUtils::GetSystemTimeString() {
    // Skeleton implementation
    return "2026-07-05 21:00:00";
}

} // namespace NtnSim::Utils
