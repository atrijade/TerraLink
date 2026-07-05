#pragma once

#include <string>
#include <unordered_map>

namespace NtnSim::Utils {

/**
 * @brief Placeholder JSON parser for loading config values.
 */
class JsonParser {
public:
    JsonParser();
    ~JsonParser();

    /**
     * @brief Parse JSON string into internal map (stub).
     */
    bool Parse(const std::string& jsonString);

    /**
     * @brief Gets a string value by key.
     */
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;

    /**
     * @brief Gets a double value by key.
     */
    double GetDouble(const std::string& key, double defaultValue = 0.0) const;

    /**
     * @brief Gets an integer value by key.
     */
    int GetInt(const std::string& key, int defaultValue = 0) const;

private:
    std::unordered_map<std::string, std::string> m_data;
};

} // namespace NtnSim::Utils
