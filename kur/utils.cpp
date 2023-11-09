#include "pch.h"
#include "utils.h"

namespace utils
{
  auto get_temp_path() -> std::optional<std::wstring>
  {
    const std::filesystem::path temp_path = std::filesystem::temp_directory_path();
    if (temp_path.empty())
    {
      return std::nullopt;
    }
    return temp_path.wstring();
  }

  auto get_ntdll() -> std::optional<HMODULE>
  {
    const HMODULE h_ntdll = GetModuleHandle(L"ntdll.dll");
    if (!h_ntdll)
    {
      return std::nullopt;
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
