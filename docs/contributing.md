# Contributing to cavOS
> First off, I'm flattered you're willing to help with cavOS' development! So, thank you in advance for that :^)

However, as you'd expect with any sort of kernel, **contributions of any sort go through a lot of auditing.** That also takes a lot of time, so please be patient as your submission is reviewed.

## Requirements
- **Proper code formatting:** 
  - Run `make format` before creating a pull request. 
  - Your submission will be checked using `make format_check` to make sure it adheres with code formatting guidelines. This is necessary so file changes are readable and code is generally consistent. 
  - For unsupported file types (like assembly, makefile, bash, etc) code quality will be reviewed by hand but keep it clean, using already pushed stuff as a reference.
- **Testing:**
  - Build cavOS and boot into it to test your changes. `make qemu` or `make vmware` are your best friend.
  - **Do NOT submit untested code**, as that wastes the contributors' time.
  - The same applies when changing utility scripts which are executed by the build system. Test those thoroughly as well.
- **Code familiarity:**
  - The biggest reason cavOS exists is **code simplicity**. I wanted its code to remain relatively simple while being powerful. It's easier to write tons of complex code in comparison to simpler code for the same goal. I generally prefer the latter.
  - Please **avoid useless assembly!** Anything that isn't plain impossible to do in C (for instance interrupt/syscall entries or some scheduler parts) you shouldn't write assembly code for. It's hard for other contributors and myself to understand and there's a much greater chance of mistakes being made.
  - Keep in mind that submissions can be rejected for any reason, even if they do something in a "better way". At the end of the day, **the maintainer gets to decide what makes it to the master branch**.

## Go for it!
With everything out of the way, good luck solider!
