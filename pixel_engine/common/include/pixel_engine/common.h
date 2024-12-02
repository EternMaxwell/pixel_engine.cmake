#pragma once

#if defined EPIX_BUILD_DLL || defined EPIX_BUILD_SHARED
#define EPIX_API __declspec(dllexport)
#elif defined EPIX_DLL || defined EPIX_SHARED
#define EPIX_API __declspec(dllimport)
#else
#define EPIX_API
#endif