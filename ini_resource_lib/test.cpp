#include "ini_resource.h"
#include <fstream>
#include <iostream>
#include <string>

// ── helpers ───────────────────────────────────────────────────────────────────

static std::string pad_to_24(const std::string& str, const std::string& color = "") {
    const auto color_reset = "\033[0m";
    if (str.size() <= 24) {
        return "> " + color + str + std::string(24 - str.size(), ' ')+color_reset;
    }
        return "> " + color+ str.substr(0, 24) + color_reset;
}
static void check(const char* op, unsigned short rc, unsigned short expected = 0)
{
    const auto GREEN = "\033[32m";
    const auto RED = "\033[31m";
    const auto color =  rc==expected ? GREEN : RED;
    std::cout   << pad_to_24(op, color)
                << "| rc=" << rc
                << (rc == expected ? " ✓" : " ✗  <-- unexpected") << '\n';
}

static std::string seed_ini(const char* path)
{
    std::ofstream f(path, std::ios::trunc);
    f << "; sample configuration\n"
      << "# Written by Giarri\n"
      << "host = localhost\n"
      << "port = 5432\n"
      << "[section]\n"
      << "key = value\n"
      << "[[color]]\n"
      << "red = rose\n";
    return path;
}

// ── main ──────────────────────────────────────────────────────────────────────

int main()
{
    const char* ini_path = "/tmp/example.ini";
    seed_ini(ini_path);

    std::cout << "=== error path: no resource loaded ===\n";
    std::string val;
    check("get value before load", get_value("key", val), 4);

    std::cout << "\n=== load_resource ===\n";
    check("load existing file ",  load_resource(ini_path));
    check("load missing file ",   load_resource("/tmp/nonexistent.ini"), 1);

    // Reload the good file so subsequent calls have a valid store.
    load_resource(ini_path);

    std::cout << "\n=== get_value ===\n";
    unsigned short rc = get_value("host", val);
    check("get 'host' ", rc);
    if (rc == 0) std::cout << "  host = " << val << '\n';

    rc = get_value("port", val);
    check("get 'port' ", rc);
    if (rc == 0) std::cout << "  port = " << val << '\n';

    check("get missing key ", get_value("timeout", val), 3);

    std::cout << "\n=== set_value ===\n";
    check("set existing 'debug' ",   set_value("debug", "true"));
    check("set in subsect. 'section.debug' ",   set_value("section.debug", "true"));
    check("set in subsubsect. foo.bar.test'' ",   set_value("foo.bar.test", "true"));
    check("set existing 'debug' ",   set_value("debug", "true"));
    check("add new key 'timeout' ",  set_value("timeout", "30"));

    // Verify persistence: reload and re-read.
    load_resource(ini_path);
    rc = get_value("debug", val);
    check("re-read 'debug' after reload ", rc);
    if (rc != 0) std::cout << "  debug = " << val << '\n';

    rc = get_value("section.key", val);
    check("re-read 'section.key' after reload ", rc);
    rc = get_value("foo.bar.test", val);
    check("re-read 'foo.bar.test' after reload ", rc);
    return 0;
}
