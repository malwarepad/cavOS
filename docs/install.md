## cavOS Building Documents

- Build environment:
    - Compile the docker environment: ``docker build docker -t os-dev``
    - Enter the build environment: 
        - Windows (PowerShell): ``docker run --rm -it -v "${pwd}:/root/env" os-dev``
        - Windows (Command Line): ``docker run --rm -it -v "%cd%":/root/env os-dev``
        - Linux (/bin/bash): ``docker run --rm -it -v "$(pwd)":/root/env os-dev``

- Compile the kernel + utilities:
    - Build the kernel + utilities: ``make build``

- Test the OS out with a QEMU emulator
    - 1. Test it out: ``qemu-system-x86_64 -cdrom cavOS.iso -m 512M``

- Clean the temporary compiled binaries
    - Use ``make clear``

### Development commands

If for whatever reason you decide to modify cavOS you could run the ``build.bat`` script from the docker folder for a quick build and execution of the OS:

``.\docker\build.bat``
``.\docker\build all``

It isn't best practice, but really useful for when you're in a hurry. 