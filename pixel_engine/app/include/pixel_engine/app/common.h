#pragma once

#if defined EPIX_BUILD_DLL
#define EPIX_API __declspec(dllexport)
#elif defined EPIX_DLL
#define EPIX_API __declspec(dllimport)
#else
#define EPIX_API
#endif