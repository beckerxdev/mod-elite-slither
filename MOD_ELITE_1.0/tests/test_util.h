#pragma once
#include <cstdio>
static int g_fail = 0, g_pass = 0;
#define CHECK(c) do { if (c) ++g_pass; else { ++g_fail; std::printf("  FALHOU %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)
#define SECTION(n) std::printf("[teste] %s\n", n)
