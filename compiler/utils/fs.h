#ifndef AYM_FS_H
#define AYM_FS_H

#if __has_include(<filesystem>)
  #include <filesystem>
  namespace fs = std::filesystem;
#elif __has_include(<experimental/filesystem>)
  #include <experimental/filesystem>
  namespace fs = std::experimental::filesystem;
#else
  #error "<filesystem> not found"
#endif

#endif // AYM_FS_H

