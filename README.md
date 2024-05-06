# Cave-Like Operating System

![Preview of the OS](https://raw.malwarepad.com/cavos/images/preview.png)

## Why make your own OS in [insert year]?
> **because I can.**

Having a good time is my drive for this project + I learn a lot of cool low level stuff alongside that! 

## Kernel status
The cavOS kernel is a monolithic x86_64 one written in relatively simple C with a few bits of Intel assembly here and there. It uses the Limine bootloader and leverages the FAT32 filesystem for booting. I try to keep the code structure clean and fairly easy to understand, while avoiding too much abstraction.

## Userspace status
Userspace is sort of broken right now. Kernel & user tasks (and scheduling) work fine, however crt0.o is non-existent, there is no stable systemcall interface and no libc... See [The x86_64 \"rewrite\"](#the-x86_64-rewrite) for more details.

## Goals

Important to mention these goals may never be satisfied, take a very long time to be completed (we're talking years down the road) or may never be done at all. Furthermore, this list won't include every feature implemented or planned and can be changed at any time...

- Life
  - [x] Lose my sanity
- Architecture: x86_64 (amd64)
  - [x] Interrupts: ISR, IRQ, GDT
  - [x] Scheduling
  - [x] Multitasking
- Device drivers
  - [x] AHCI
  - [x] PCI read/write
  - [ ] Mouse
- Networking stack
  - [ ] Improve reliability & security
  - [ ] Sockets
  - [ ] DNS
  - [ ] HTTP
  - [ ] TLS
- Userspace 
  - [x] cavOS Specific Toolchain
  - [x] ELF64 parsing
  - [x] Port musl (libc)
- Interfaces
  - [x] Kernel shell
  - [ ] Bash port
  - [ ] Vim port
  - [ ] IRC client
- Graphics
  - [ ] idk (seriously)

## The x86_64 "rewrite"
Saturday March 2nd of 2024. Through many workarounds, "bad" decisions and an overbearing "just-works" mentality, I had pieced together a purely x86 (32-bit) kernel that could unreliably fuel userspace applications. Still holding onto old code (from back when I barely understood simple concepts, like say paging), outdated libraries and a lot of other stuff. It *sometimes* worked, but I was not satisfied.

5:00PM; That afternoon I decided to start a lengthy process of migrating everything to the x86_64 architecture and ironing out plenty of reliability issues, which made for actual nightmares to debug. I basically reached a certain point to understand that quick & dity solutions only lead to completely avoidable mistakes, which were extremely hard to pinpoint after tremendous amounts of abstractions were added.

As of the current commit, a lot of leftovers from x86 are present. While I migrate/rewrite everything left, this repository might contain a few unneeded stuff. Just know that those are not "unusable", they used to be part of the featureset and worked fine.

## Compiling
Everything about this can be found over at [install.md](docs/install.md). Go there for more info about building the OS correctly, cleaning unused binaries and other stuff. 

## Credits
- [G-9](https://nr9.online/): ASCII art on fetch!

## License
This project is licensed under GPL v3 (GNU General Public License v3.0). For more information go to the LICENSE file.
