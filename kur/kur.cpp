#include "pch.h"
#include "kur.h"

auto main() -> int
{
  std::unique_ptr<kur_t> kur;
  try
  {
    kur = std::make_unique<kur_t>(L"EchoDrv", L"\\Device\\EchoDrv");
  }
  catch (const std::exception& e)
  {
    std::cout << e.what() << std::endl;
  }

  const HANDLE h_device = kur->query_device_handle();
  printf("device_handle: %p\n", h_device);

  int num = 85687684;

  int buf = 0;
  auto ok = kur->read(&num, &buf, sizeof(buf), GetCurrentProcess());
  if (!ok)
  {
    std::cout << "read failed" << std::endl;
  }

  std::cout << buf << std::endl;

  std::cin.get();
  return 0;
}

auto kur_t::get_process_handle(DWORD pid, ACCESS_MASK access_mask) -> std::optional<HANDLE>
{
  return vul_driver.ioctl_get_process_handle(pid, access_mask);
}

// not tested yet
auto kur_t::read(void* address, void* buffer, size_t buffer_size, HANDLE h_target_process) -> BOOL
{
  return vul_driver.ioctl_mm_copy_virtual_memory(address, buffer, buffer_size, h_target_process);
}

auto kur_t::query_device_handle() const -> HANDLE
{
  return vul_driver.h_device;
}
