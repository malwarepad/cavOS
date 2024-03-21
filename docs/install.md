# Building Documents

Compiling an operating system is not an easy task... The build environment I require for building cavOS is as simple as it gets, only needing a few UNIX utilities and obviously a cross compiler...

> **Warning!** If any step fails, do NOT proceed... Start over (double checking your dependencies) and if the issue persists, create an issue reporting it. Remember, this project is still in development!

## Requirements
- \[*cross compiler*]: The kernel is written in C.
- nasm: The kernel includes some small assembly files ~~unfortunately~~.
- parted: For creating the boot disk.
- mkdosfs: For creating the FAT32 filesystem inside that disk.
- fatlabel: For labeling that disk as CAVOS.

Most required packages for compiling on specific distros (listed [below](#dependencies-gnulinux)) are specifically for building the cross-compiler... More info [here](https://wiki.osdev.org/GCC_Cross-Compiler).

## Dependencies: GNU/Linux
Compiling cavOS will work on all Linux distributions, but I've only **done testing** with the options that are **bold**, so pick wisely...

- **Ubuntu:** `sudo apt install -y nasm dosfstools parted bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev parted dosfstools build-essential`
- **Fedora:** `sudo dnf install -y nasm gcc xorriso parted dosfstools make bison flex gmp-devel libmpc-devel mpfr-devel texinfo`
- **Arch:** `sudo pacman -S --noconfirm nasm xorriso gcc parted dosfstools base-devel gmp libmpc mpfr`

If everything else fails, you got [distrobox](https://github.com/89luca89/distrobox).

## Dependencies: Microsoft Windows
Windows is objectively a shitshow when doing anything even slightly UNIX-related. If you want to get your hands dirty with Cygwin, then that's your choice. I'd suggest going with **WSL2, fully updated**. It's still going to be **extremely buggy**, at least from my experience. You have the option of creating a **headless Linux VM** inside software like VMware or VirtualBox, which might be your best option...

## Dependencies: macOS
I have not done any testing inside macOS. When anyone does so, please edit this file accordingly.

I can imagine that the cross compiler would work fine, but installing Limine by creating loopback devices wouldn't work. **The recommendation of Docker or even virtual machines persists to this OS too.**

## Get the cross-compiler
If you got your dependencies sorted out, then this step is relatively simple! Just run `make tools`.

## Building
This is an operating system, meaning that it has to be burnt into some sort of medium (disk, USB, floppy drive, somewhere!)

> **Warning!** In case you want to try cavOS on real hardware, while nobody is liable for any damage it might cause, many features might not be available due to a lack of drivers!

For building there are two options (although the second is mostly geared towards developers/tinkerers):
- **Build disk.img, a fully featured pack of cavOS that includes Limine, the kernel and any software: `make disk`.**
- Build only the kernel.bin file, cavOS kernel: `make kernel`.

## Testing
- **QEMU:** `make qemu` will launch QEMU with the recommended options on the `disk.img` disk image.
- **VMware products:** `make vmware` will convert `disk.img` -> `disk.vmdk`.
- **VirtualBox:** can handle the `disk.vmdk` file, so you can use the same disk image as VMware.
