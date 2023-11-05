#include "utils.h"

namespace utils
{
  auto get_temp_path() -> std::wstring
  {
    const std::filesystem::path temp_path = std::filesystem::temp_directory_path();
    if (temp_path.empty())
    {
      std::cerr << "cannot get temp path" << std::endl;
      return L"";
    }
    return temp_path.wstring();
  }

  auto get_ntdll() -> HMODULE
  {
    const HMODULE h_ntdll = GetModuleHandle(L"ntdll.dll");
    if (!h_ntdll)
    {
      std::cerr << "couldn't get ntdll handle" << std::endl;
      return NULL;
    }
    return h_ntdll;
  }
}
