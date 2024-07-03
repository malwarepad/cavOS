[![GitHub top language](https://img.shields.io/github/languages/top/malwarepad/cavOS?logo=c&label=)](https://github.com/malwarepad/cavOS/blob/master/src/kernel/Makefile)
[![GitHub license](https://img.shields.io/github/license/malwarepad/cavOS)](https://github.com/malwarepad/cavOS/blob/master/LICENSE)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/e78ad48f394f46d1bb98f1942c7e1f21)]
[![GitHub contributors](https://img.shields.io/github/contributors/malwarepad/cavOS)](https://github.com/malwarepad/cavOS/graphs/contributors)
[![GitHub commit activity (branch)](https://img.shields.io/github/commit-activity/m/malwarepad/cavOS)](https://github.com/malwarepad/cavOS/commits)

# Table of Contents

1. [Cave-Like Operating System](#cave-like-operating-system)
2. [Why make your own OS in [insert year]?](#why-make-your-own-os-in-insert-year)
3. [Kernel status](#kernel-status)
4. [Userspace status](#userspace-status)
5. [Goals](#goals)
   - [Life](#life)
   - [Architecture: x86_64 (amd64)](#architecture-x86_64-amd64)
   - [Device drivers](#device-drivers)
   - [Networking stack](#networking-stack)
   - [Userspace](#userspace)
   - [Interfaces](#interfaces)
   - [Graphics](#graphics)
6. [Compiling](#compiling)
7. [Credits](#credits)
8. [Archive: The x86_64 "rewrite"](#archive-the-x86_64-rewrite)
9. [License](#license)

 

# Cave-Like Operating System
![Preview of the OS](https://raw.malwarepad.com/cavos/images/preview.png)

## Why make your own OS in [insert year]?
> **Because I can.**

Having a good time is my drive for this project + I learn a lot of cool low level stuff alongside that! 

## Kernel status
The cavOS kernel is a monolithic x86_64 one written in relatively simple C with a few bits of Intel assembly here and there. It uses the Limine bootloader and leverages the FAT32 filesystem for booting. I try to keep the code structure clean and fairly easy to understand, while avoiding too much abstraction.

## Userspace status
Userspace is my primary focus at the time being, with the kernel being *quite* stable. I'm trying to make this OS as close to Linux as I can, while adding my own stuff on top of it. This is visible with the system calls that are exactly like Linux's. That isn't random, I want cavOS to be as binary compatible with it as possible!

## Goals

Important to mention these goals may never be satisfied, take a very long time to be completed (we're talking years down the road) or may never be done at all. Furthermore, this list won't include every feature implemented or planned and can be changed at any time...

- Life
  - [x] Lose my sanity
- Architecture: x86_64 (amd64)
  - [x] Interrupts: ISR, IRQ, GDT
  - [x] Ring3: Scheduling, ELF64, SSE, paging, `systemcall`
  - [x] Generic: RTC
- Device drivers
  - [x] Storage: AHCI
  - [x] Networking: RTL8139, RTL8169
  - [x] Generic: PS/2 Keyboard, serial port
  - [ ] Generic: Mouse
- Networking stack
  - [x] Layer3: IPv4, ARP, ICMP
  - [x] Layer4: UDP, TCP
  - [x] Layer5: DHCP
  - [ ] Layer6: TLS
  - [ ] Layer7: DNS, HTTP
  - [x] OS Interface: Sockets
  - [ ] Generic: Improve reliability & security
- Userspace
  - [x] Toolchain: cavOS-specific
  - [x] Libraries: musl(libC)
  - [ ] Linking: Dynamic
  - [ ] System calls: signals, sockets, filesystem, etc...
- Interfaces
  - [x] Kernel: Embedded shell
  - [ ] Kernel: Good panic screens
  - [ ] Ports: Bash, coreutils, vim, binutils, gcc
  - [ ] Desires: Cool fetch, some damn IRC client
- Graphics
  - [x] Framebuffer: Use in kernel
  - [ ] Framebuffer: Pass to userspace
  - [ ] Xorg or smth? idk (seriously)

## Compiling
Everything about this can be found over at [install.md](docs/install.md). Go there for more info about building the OS correctly, cleaning unused binaries and other stuff. 

## Credits
- [G-9](https://nr9.online/): ASCII art on fetch!

## Archive: The x86_64 "rewrite"
Saturday March 2nd of 2024. Through many workarounds, "bad" decisions and an overbearing "just-works" mentality, I had pieced together a purely x86 (32-bit) kernel that could unreliably fuel userspace applications. Still holding onto old code (from back when I barely understood simple concepts, like say paging), outdated libraries and a lot of other stuff. It *sometimes* worked, but I was not satisfied.

5:00PM; That afternoon I decided to start a lengthy process of migrating everything to the x86_64 architecture and ironing out plenty of reliability issues, which made for actual nightmares to debug. I basically reached a certain point to understand that quick & dity solutions only lead to completely avoidable mistakes, which were extremely hard to pinpoint after tremendous amounts of abstractions were added.

## License
This project is licensed under GPL v3 (GNU General Public License v3.0). For more information go to the [LICENSE](LICENSE) file.
