#pragma once
#include <string>
namespace me {
// Valida "ip:porta" / "host:porta" (aceita prefixo ws:// e sufixo /slither). A porta é obrigatória.
// Não inventa servidores: o endereço deve ser de um servidor real do Slither.io.
bool parseServer(const std::string& in, std::string& host, int& port, std::string& err);
}  // namespace me
