#include "ini_resource.h"

#include <filesystem>
#include <map>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>
#include <ostream>
#include <ranges>
#include <string>

using namespace std;
class Node;
using Path = vector<string>;

static string trim(const string& line);
static bool is_line_kv_store(const string& line);
static bool is_section_header(const string& line);
static string read_key(const string& line);
static string read_value(const string& line);
static string get_section_name(const string& line);
static bool flush_to_disk();
static Path split_dots(const string& s);
template < class T > std::ostream& operator << (std::ostream& os, const std::vector<T>& v);

// ── internal state ────────────────────────────────────────────────────────────
static vector<Node*> g_node_path;
static Node* g_current_Node;
static string g_path;

// ── internal types ────────────────────────────────────────────────────────────
class Node {
    const string section_name;
    vector<string> raw_lines{};
    map<string, string> values{};
    map<string, Node*> sections{};
    const int section_level = 0;

public:
    Node(string section_name) : section_name(std::move(section_name)) {}

    Node(string section_name, int section_level)
        : section_name(std::move(section_name)),
          section_level(section_level) {}

    void parse_line(string& line) {
        if (is_section_header(trim(line))) {
            line = trim(line);
            int new_section_level;
            new_section_level = ranges::count(line, '[');
            auto new_section = get_section_name(line);
            if (this->section_level < new_section_level) {
                Node* n = new Node{new_section, new_section_level};
                g_current_Node = n;
                sections[new_section] = n;
                g_node_path.push_back(n);
            }
            if (this->section_level >= new_section_level) {
                while ((int)g_node_path.size() > new_section_level) {
                    g_node_path.pop_back();
                }
                auto tail = g_node_path.back();
                tail->add_section(new_section);
            }
            return;
        }
        raw_lines.push_back(line);
        line = trim(line);
        if (is_line_kv_store(line)) {
            auto key = read_key(line);
            auto value = read_value(line);
            values[key] = value;
        }

    }

    bool has_section(const string& section_name) {
        return sections.find(section_name) != sections.end();
    }

    Node* get_section(const string& section_name) {
        return sections[section_name];
    }

    bool contains_key(const string& key) {
        return values.contains(key);
    }

    string get_value(const string& key) {
        return values[key];
    }

    void set_value(const string& key, const string& value) {
        auto pos = values.find(key);
        if (pos == values.end()) {
            values[key] = value;
            raw_lines.push_back((key + '=') + value);
            return;
        }
        for (auto& raw_line : raw_lines) {
            if (is_line_kv_store(raw_line)) {
                if (read_key(raw_line) == key) {
                    raw_line = (key + '=') + value;
                    break;
                }
            }
        }
    }

    Node* add_section(const string& new_section) {
        Node* n = new Node{new_section, this->section_level + 1};
        sections[new_section] = n;
        g_current_Node = n;
        g_node_path.push_back(n);
        return n;
    }

    friend std::ostream& operator<<(ostream& os, const Node& obj) {
        os << string(obj.section_level, '[') << obj.section_name << string(obj.section_level, ']') << (0==obj.section_level?"":"\n");
        os << obj.raw_lines; //values and comments
        for (auto val: obj.sections | views::values)
            os << *val;
        return os;
    }

private:
    bool operator==(const Node& n) const { return section_name == n.section_name; }
    bool operator<(const Node& n) const { return section_name < n.section_name; }
};

// ── public API ────────────────────────────────────────────────────────────────
/**
 * Loads and parse a INI resource file located in a standard Linux filesystem
 * @param path path of the INI file to load. The path is a Linux path located in the same system
 * where the library is executed
 * @return  0   - load succeeds
 *          1   - read error
 *          255 - generic error
 */
unsigned short load_resource(const std::string& path)
{
    try {
        std::ifstream in(path);
        if (!in) return 1;

        g_current_Node = new Node{""};
        g_node_path = {g_current_Node};
        g_path = path;

        std::string line;
        while (std::getline(in, line)) {
            g_current_Node->parse_line(line);
        }
        return !in.eof();
    } catch (...) { return 255; }
}


/**
 * Retrieves the value of a key in a previous loaded INI file
 * @param key   the key to search for
 * @param value the address of the string with the value if found
 * @return  0   - success\n
 *          3   - missing key\n
 *          4   - no resource file has been loaded yet\n
 *          255 - generic error
 */
unsigned short get_value(const std::string& key, std::string& value)
{
    try {
        if (g_path.empty()) return 4;
        auto path = split_dots(key);
        if (path.empty()) return 3;
        auto head = g_node_path.front();
        for (int i = 0; i < (int)path.size() - 1; i++) {
            if (const string& section = path.at(i); head->has_section(section))
                head = head->get_section(section);
            else
                return 3;
        }
        const auto& leaf = path.back();
        if (head->contains_key(leaf)) {
            value = head->get_value(leaf);
            return 0;
        }
        return 3;
    } catch (...) { return 255; }
}

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
unsigned short set_value(const std::string& key, const std::string& value)
{
    try {
        if (g_path.empty()) return 4;
        auto parts = split_dots(key);
        if (parts.empty()) return 255;

        const Path& path = parts;
        auto head = g_node_path.front();
        for (int i = 0; i < (int)path.size() - 1; i++) {
            if (const string& section = path.at(i); head->has_section(section))
                head = head->get_section(section);
            else
                head = head->add_section(section);
        }
        const auto leaf = path.back();
        head->set_value(leaf, value);

        return flush_to_disk() ? 0 : 255;
    } catch (...) { return 255; }
}

// ── helper functions ──────────────────────────────────────────────────────────
static string trim(const string &line) {
    auto b = line.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) {
        return {};
    } else {
        auto e = line.find_last_not_of(" \t\r\n");
        return line.substr(b, e - b + 1);
    }
}

bool is_line_kv_store(const string &line) {
    if (line.empty() || line[0] == ';' || line[0] == '#' || line[0] == '[') return false;
    auto eq = line.find('=');
    if (eq == std::string::npos) return false;
    return true;
}

static bool is_section_header(const string &line) {
    if (!line.empty() && line[0]=='[') return true;
    return false;
}

static string read_key(const string &line) {
    auto eq = line.find('=');
    return trim(line.substr(0, eq));
}

static string read_value(const string &line) {
    auto eq = line.find('=');
    return trim(line.substr(eq + 1));
}

static string get_section_name(const string &line) {
    auto b = line.find_last_of('[');
    auto e = line.find_first_of(']');
    if (b == std::string::npos || e == std::string::npos) return "";
    return line.substr(b+1, e - b - 1);
}

static Path split_dots(const std::string& s)
{
    Path parts;
    std::istringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, '.'))
        if (!tok.empty()) parts.push_back(tok);
    return parts;
}

//an optimization would be of writing only the bytes needed -> see
// Source - https://stackoverflow.com/a/63152883
// Posted by john, modified by community. See post 'Timeline' for change history
// Retrieved 2026-06-10, License - CC BY-SA 4.0
// std::ofstream ofs {"foo", std::ios::in|std::ios::out|std::ios::binary};
static bool flush_to_disk()
{
    std::ofstream out(g_path, std::ios::trunc);
    if (!out) return false;
    out << *g_node_path.front();
    return true;
}

// Source - https://stackoverflow.com/a/4077759
// Posted by Jason Iverson, modified by community. See post 'Timeline' for change history
// Retrieved 2026-06-10, License - CC BY-SA 2.5
template < class T >
std::ostream& operator << (std::ostream& os, const std::vector<T>& v)
{
    for (auto ii = v.begin(); ii != v.end(); ++ii)
    {
        os << *ii << "\n";
    }
    return os;
}