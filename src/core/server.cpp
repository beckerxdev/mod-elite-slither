#include "server.h"
#include <cctype>
#include <cstdlib>
namespace me {
bool parseServer(const std::string& in, std::string& host, int& port, std::string& err) {
    std::string s = in;
    while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
    if (s.rfind("ws://", 0) == 0) s = s.substr(5);
    auto sl = s.find('/'); if (sl != std::string::npos) s = s.substr(0, sl);
    if (s.empty()) { err = "Informe IP:porta de um servidor do Slither.io."; return false; }
    auto c = s.rfind(':');
    if (c == std::string::npos) { err = "Formato esperado: IP:porta (ex.: 1.2.3.4:444)."; return false; }
    host = s.substr(0, c);
    std::string ps = s.substr(c + 1);
    if (host.empty() || ps.empty() || ps.size() > 5) { err = "Endereço ou porta inválidos."; return false; }
    for (char ch : ps) if (!std::isdigit((unsigned char)ch)) { err = "Porta deve ser numérica."; return false; }
    port = std::atoi(ps.c_str());
    if (port < 1 || port > 65535) { err = "Porta fora do intervalo 1-65535."; return false; }
    for (char ch : host) if (!(std::isalnum((unsigned char)ch) || ch == '.' || ch == '-')) { err = "Endereço contém caracteres inválidos."; return false; }
    return true;
}
}  // namespace me
