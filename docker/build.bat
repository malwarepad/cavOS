@echo off

docker run --rm -it -v "%cd%":/root/env os-dev make build
qemu-system-x86_64 -cdrom cavOS.iso -m 512M