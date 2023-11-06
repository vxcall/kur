#include "kur.h"

auto retrieve_device_handle(const std::wstring& device_name, ACCESS_MASK access_mask) -> HANDLE;

auto main() -> int
{
  kur::init();
  auto h_device = retrieve_device_handle(L"\\Device\\EchoDrv", GENERIC_READ | GENERIC_WRITE);

  printf("device_handle: %p\n", h_device);

  kur::cleanup(h_device);
  std::cin.get();
  return 0;
}

auto kur::init() -> BOOL
{
  if (!vul_driver::install_driver())
  {
    std::cerr << "couldn't deploy the driver" << std::endl;
    return FALSE;
  }

  auto status = vul_driver::setup_reg_key();
  if (!status)
  {
    std::cerr << "couldn't setup the registry" << std::endl;
    vul_driver::uninstall_driver();
    return FALSE;
  }

  status = vul_driver::load_driver();
  if (!status)
  {
    std::cerr << "couldn't start the service" << std::endl;
    vul_driver::uninstall_driver();
    vul_driver::delete_reg_key();
    return FALSE;
  }

  return TRUE;
}

auto kur::cleanup(HANDLE h_device) -> BOOL
{
  const BOOL st = vul_driver::cleanup_reg_driver(h_device);
  if (!st)
  {
    std::cerr << "couldn't cleanup the driver" << std::endl;
    return FALSE;
  }
  return TRUE;
}


// L"\\Device\\echo", GENERIC_READ | GENERIC_WRITE
auto retrieve_device_handle(const std::wstring& device_name, ACCESS_MASK access_mask) -> HANDLE
{
  NTSTATUS status;
  HANDLE h_device;
  OBJECT_ATTRIBUTES obj_attr;
  UNICODE_STRING uni_device_name;
  IO_STATUS_BLOCK io_status_block;

  RtlInitUnicodeString(&uni_device_name, device_name.c_str());

  InitializeObjectAttributes(&obj_attr, &uni_device_name,
                             OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

  const ULONG share_access = 0;
  const ULONG open_options = 0;

  status = NtOpenFile(&h_device,
                      access_mask,
                      &obj_attr,
                      &io_status_block,
                      share_access,
                      open_options);

  if (!NT_SUCCESS(status))
  {
    std::cerr << "Failed to open handle. Status code: " << std::hex << status << std::endl;
    return nullptr;
  }
  // This handle has to be closed with CloseHandle(device_handle);
  return h_device;
}
