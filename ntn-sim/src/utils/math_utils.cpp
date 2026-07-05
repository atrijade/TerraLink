#include "ntn-sim/utils/math_utils.h"

namespace NtnSim::Utils {

double MathUtils::CalculateDistance(const Position3D& pos1, const Position3D& pos2) {
    (void)pos1;
    (void)pos2;
    // Skeleton implementation
    return 0.0;
}

Position3D MathUtils::GeodeticToEcef(const GeodeticPosition& geo) {
    (void)geo;
    // Skeleton implementation
    return Position3D{};
}

double MathUtils::CalculateElevationAngle(const Position3D& groundStation, const Position3D& satellite) {
    (void)groundStation;
    (void)satellite;
    // Skeleton implementation
    return 90.0;
}

} // namespace NtnSim::Utils
