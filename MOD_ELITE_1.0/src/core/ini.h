#pragma once
// Armazenamento chave=valor simples e leve (sem dependências). Gravação atômica.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
namespace me {
class Ini {
public:
    bool load(const std::string& path) {
        std::ifstream f(path);
        if (!f) return false;
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty() || line[0] == '#') continue;
            auto p = line.find('=');
            if (p == std::string::npos) continue;
            kv_[line.substr(0, p)] = line.substr(p + 1);
        }
        return true;
    }
    bool save(const std::string& path) const {
        std::string tmp = path + ".tmp";
        {
            std::ofstream f(tmp, std::ios::trunc);
            if (!f) return false;
            for (auto& e : kv_) f << e.first << '=' << e.second << '\n';
            f.flush();
            if (!f.good()) return false;
        }
        std::remove(path.c_str());  // Windows: rename não sobrescreve
        return std::rename(tmp.c_str(), path.c_str()) == 0;
    }
    void setInt(const std::string& k, long long v) { kv_[k] = std::to_string(v); }
    void setBool(const std::string& k, bool v) { kv_[k] = v ? "1" : "0"; }
    void setFloat(const std::string& k, double v) { char b[48]; std::snprintf(b, sizeof b, "%.6g", v); kv_[k] = b; }
    void setStr(const std::string& k, std::string v) {
        for (auto& c : v) if (c == '\n' || c == '\r') c = ' ';
        kv_[k] = v;
    }
    long long getInt(const std::string& k, long long d) const {
        auto it = kv_.find(k); if (it == kv_.end() || it->second.empty()) return d;
        char* e = nullptr; long long v = std::strtoll(it->second.c_str(), &e, 10);
        return (e && *e == 0) ? v : d;
    }
    double getFloat(const std::string& k, double d) const {
        auto it = kv_.find(k); if (it == kv_.end() || it->second.empty()) return d;
        char* e = nullptr; double v = std::strtod(it->second.c_str(), &e);
        return (e && *e == 0) ? v : d;
    }
    bool getBool(const std::string& k, bool d) const { return getInt(k, d ? 1 : 0) != 0; }
    std::string getStr(const std::string& k, const std::string& d) const {
        auto it = kv_.find(k); return it == kv_.end() ? d : it->second;
    }
private:
    std::map<std::string, std::string> kv_;
};
}  // namespace me
