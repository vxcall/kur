# kur
A library simply offers the ability to kernel-mode read / write memory from user-mode using vulnerable signed driver.
It's just utilizing MmCopyVirtualMemory in kernel mode so you can read / write any user-mode memory without having to worry about the protection the memory page has. Also it has a function to obtain process handle of given pid as a side arm. not interesting that much tho.

This project was created for study purposes, and it is not recommended to use it outside of a virtual machine.

# background
I've seen that this specific driver has privilege escalation vulnerability in uc forum. 
So I started reversing the driver myself and indeed the driver is quite readable and doesnt have access control over its strong ioctls.
