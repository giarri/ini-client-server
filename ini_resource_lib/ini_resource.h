#pragma once
#include <string>

/**
 * Loads and parse a INI resource file located in a standard Linux filesystem
 * @param path path of the INI file to load. The path is a Linux path located in the same system
 * where the library is executed
 * @return  0   - load succeds
 *          1   - read error
 *          255 - generic error
 */
unsigned short load_resource(const std::string& path);

/**
 * Retrieves the value of a key in a previous loaded INI file
 * @param key   the key to search for
 * @param value the address of the string with the value if found
 * @return  0   - success
 *          3   - missing key
 *          4   - no resource file has been loaded yet
 *          255 - generic error
 */
unsigned short get_value(const std::string& key, std::string& value);

/**
 * Allows the application to store the value of a key in a previous loaded INI file.
 * This adds or replace the new key/value pair both in the volatile memory and in the
 * INI file on the system
 * @param key   the key to substitute
 * @param value the new value
 * @return  0   - success
 *          4   - no resource file has been loaded yet
 *          255 - generic error
 */
unsigned short set_value(const std::string& key, const std::string& value);
