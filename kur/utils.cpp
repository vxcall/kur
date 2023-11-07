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

  // used to check if key exists or not
  auto open_reg_key(HKEY h_key_root, LPCWSTR sub_key) -> LSTATUS
  {
    HKEY h_key;
    const LSTATUS status = RegOpenKeyExW(h_key_root, sub_key, 0, KEY_READ, &h_key);

    if (status != ERROR_SUCCESS)
    {
      return status;
    }

    RegCloseKey(h_key);
    return status;
  }
}
