## Building Documents

- [Building Documents](#building-documents)
	- [Requirements](#requirements)
		- [Ubuntu](#ubuntu)
		- [Fedora](#fedora)
		- [Arch](#arch)
	- [Building](#building)
	- [Testing](#testing)

### Requirements
- grub-mkrescue
- xorriso
- gcc
- ld
- nasm

#### Ubuntu
`sudo apt install -y nasm build-essential binutils grub-common grub-pc-bin xorriso`

#### Fedora
`sudo dnf install -y nasm grub gcc xorriso`

#### Arch
`sudo pacman -S --noconfirm nasm grub xorriso gcc`

### Building
To build cavOS, use the following command: `make build`, after doing `make clear`.

### Testing
To test your cavOS kernel using grub, run the qemu.sh script with no arguments:
`./qemu.sh`
