#pragma once

#if defined(_WIN32) || defined(_WIN64)
  #if defined(GDIV_EXPORTS)
    #define GDIV_API __declspec(dllexport)
  #else
    #define GDIV_API __declspec(dllimport)
  #endif
#else
  #if __GNUC__ >= 4
    #define GDIV_API __attribute__((visibility("default")))
  #else
    #define GDIV_API
  #endif
#endif