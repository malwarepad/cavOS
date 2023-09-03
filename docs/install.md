 ## Building Documents

- [Building Documents](#building-documents)
	- [Requirements](#requirements)
		- [Ubuntu](#ubuntu)
		- [Fedora](#fedora)
		- [Arch](#arch)
	- [Get the cross-compiler](#get-the-cross-compiler)
	- [Building](#building)
	- [Testing](#testing)

### Requirements
- grub-mkrescue
- xorriso
- gcc
- ld
- nasm

#### Ubuntu
`sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev`

#### Fedora
`sudo dnf install -y nasm grub gcc xorriso`

#### Arch
`sudo pacman -S --noconfirm nasm grub xorriso gcc`

### Get the cross-compiler
`make tools`

### Building
For building you do have a lot of options:
- Create a FAT32 disk image that supports reading: `make disk`
- Create an ISO 9660 image with lack of disk operations (NOT recommended): `make iso`

For cleaning your image up you can use `make clean`

### Testing
To test your cavOS kernel using grub, run the qemu.sh script with no arguments:
`./qemu.sh`
