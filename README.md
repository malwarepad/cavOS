# Cave-Like Operating System

![Preview of the OS](./docs/preview.png)

## Table Of Contents

- [Cave-Like Operating System](#cave-like-operating-system)
  - [Table Of Contents](#table-of-contents)
  - [Purpose](#purpose)
  - [Status](#status)
  - [Future plans](#future-plans)
  - [Goals](#goals)
  - [Compiling](#compiling)
  - [License](#license)

## Purpose

**This is an OS I am making in my free time purely for fun.** I am learning a bunch of things about OS development while writing it. I had no plans to make it a public project, but after a few suggestions I decided to do it. 

## Status

**For now it has a basic, monolithic kernel and a few drivers built into the kernel**. In addition the whole shell is built into the kernel creating a very compact operating system. 

The utilities are really simple, as **I have almost zero plans to make it bloated with useless apps/games**. If I ever do it in the future I'll create patches or generally have an option to exclude it from even compiling. It's important to mention that most of the software included doesn't use the full potential of the kernel. **You're supposed to write the stuff you want for yourself.** 

***To conclude everything, it needs a lot of polishing, time, and lines of code to be an OS that you could dual boot with your current installation.*** 

## Future plans

I'm thinking of extending this project to a full operating system once I acquire all of the knowledge required. However this will purely come from experience as my primary goal from this os is to learn os development. 

## Goals

Important to mention these goals may never be satisfied, take a very long time to be completed (we're talking years down the road) or may never be done at all.

| Goal                         | Status   |
| ---------------------------- |:--------:|
| Text Driver                  | Done     |
| Keyboard Driver              | Done     |
| Shell Bases                  | Done     |
| Simple Shell                 | Almost   |
| Filesystem (Hard)            | Not done |
| Display Driver (Hard)        | Done     |
| Full GUI (Impossible)        | Not done |
| Network Driver               | Not done |

## Compiling

Everything about this can be found over on `docs/install.md`. Go there for more info about building the OS correctly, cleaning unused binaries and other stuff. 

## License

This project is licensed under GPL v3 (GNU General Public License v3.0). For more information go to the LICENSE file.
