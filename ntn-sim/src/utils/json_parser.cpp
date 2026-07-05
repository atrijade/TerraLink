#include "ntn-sim/utils/json_parser.h"

namespace NtnSim::Utils {

JsonParser::JsonParser() {}

JsonParser::~JsonParser() {}

bool JsonParser::Parse(const std::string& jsonString) {
    (void)jsonString;
    // Skeleton implementation
    return true;
}

std::string JsonParser::GetString(const std::string& key, const std::string& defaultValue) const {
    (void)key;
    return defaultValue;
}

double JsonParser::GetDouble(const std::string& key, double defaultValue) const {
    (void)key;
    return defaultValue;
}

int JsonParser::GetInt(const std::string& key, int defaultValue) const {
    (void)key;
    return defaultValue;
}

} // namespace NtnSim::Utils
