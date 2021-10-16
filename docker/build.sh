docker run --rm -it -v "$(pwd)":/root/env os-dev make build
qemu-system-x86_64 -cdrom cavOS.iso -m 512M