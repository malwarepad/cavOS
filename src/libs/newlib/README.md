# Newlib for cavOS
Newlib's compilation proccess is an absolute mess... However, it was too good of a LibC so I used it anyways. Here is `newlib-4.1.0` which I got from ftp://sourceware.org/pub/newlib/newlib-4.1.0.tar.gz...

## Warning
This file mostly consists of dumps straight from my notes, don't expect it to be too legible! **Please use the script or (even better) let everything at the mercy of the root Makefile!**

## Requirements
Newlib is [incredibly pesky](https://wiki.osdev.org/Porting_Newlib) about the compilation environment, so you will need **exactly** the following:
- Automake v1.11: https://ftp.gnu.org/gnu/automake/automake-1.11.tar.gz
- Autoconf v2.65: https://ftp.gnu.org/gnu/autoconf/autoconf-2.65.tar.gz
> Obviously ensure these are put in path correctly: `export PATH=~/bin/bin:$PATH`

## Autotools mess
- `autoconf` inside `newlib/libc/sys`
- `autoreconf` inside `newlib/libc/sys/cavos`

## Building
> Considering an output directory of `~/fr`!

```
mkdir build && cd build
../newlib-4.1.0/configure --prefix=/home/panagiotis/fr --target=i686-cavos
make all
make install
```