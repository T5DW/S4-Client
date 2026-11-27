#include "../common.hpp"
#pragma once
#if defined(_WIN32) && defined(S4_BUILDING_CLIENT)
#define S4_CLIENT_API __declspec(dllexport)
#else
#define S4_CLIENT_API
#endif
extern "C" S4_CLIENT_API int S4ClientStartComponents();
extern "C" S4_CLIENT_API void S4ClientStopComponents();
