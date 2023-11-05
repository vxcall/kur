#include "kur.h"

auto main() -> int
{
  if (!vul_driver::install_driver())
  {
    std::cerr << "couldn't deploy the driver" << std::endl;
    return 1;
  }
  do
  {
    auto status = vul_driver::setup_reg_key();
    if (!status)
    {
      std::cerr << "couldn't setup the registry" << std::endl;
      break;
    }
    status = vul_driver::load_driver();
    if (!status)
    {
      std::cerr << "couldn't start the service" << std::endl;
      break;
    }
  }
  while (false);
  std::cout << "driver is loaded correctly." << std::endl;
  BOOL st = vul_driver::cleanup_reg_driver();
  if (!st)
  {
    std::cerr << "couldn't cleanup the driver" << std::endl;
    return 1;
  }
  std::cout << "driver and reg is cleaned up correctly" << std::endl;
  std::cin.get();
  return 0;
}
