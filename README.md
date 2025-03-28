![kur_header](https://github.com/user-attachments/assets/85cfdef3-161e-42df-ac85-e7a8bc5b282a)

# kur
A code simply offers the ability to kernel-mode read / write memory from user-mode using vulnerable signed driver.
It's simply utilizing MmCopyVirtualMemory, which is undocumented yet one of the most common API, in kernel mode so you can read / write any user-mode memory without having to worry about the protection the memory page has. Besides that, it has a function to obtain process handle of given pid as a side arm. Since handle creation is conducted in kernel-mode, standard access checks and callbacks won't kick in i believe.

This project was created for study purposes, and it is not recommended to use it outside of a virtual machine.

# background
I've seen that this specific driver has privilege escalation vulnerability in uc forum. 
So I started reversing the driver myself and indeed the driver doesnt have access control over its strong ioctls.

# the vulnerability

This project utilizes the vulnerability that was reported as CVE-2023-38817. 
https://www.loldrivers.io/drivers/afb8bb46-1d13-407d-9866-1daa7c82ca63/

# How to use
Since I was too lazy to implement properly u have to just include everything and compile.
all functionalities are encapsulated in kur_t class.

https://github.com/pseuxide/kur/blob/cd3da42b8a146af3ca83d5f95970b030bd55585f/kur/kur.h#L5-L14
