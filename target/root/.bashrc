# System wide environment variables and startup programs should go into
# /etc/profile.  Personal environment variables and startup programs
# should go into ~/.bash_profile.  Personal aliases and functions should
# go into ~/.bashrc

# Provides colored /bin/ls and /bin/grep commands.  Used in conjunction
# with code in /etc/profile.
source /etc/profile

# Only if nothing has already set it (like a terminal emulator)
if [ -z "${TERM}" ] || [ "${TERM}" == "dumb" ]; then
	export TERM=linux
fi

alias ls='ls --color=auto'
alias grep='grep --color=auto'
alias dir="dir --color=auto"
alias mpv="mpv -no-audio"

# some helpful aliases
alias a="rm -f /var/log/Xorg.0.log && echo d > /sys/cavosConsole && xinit && echo e > /sys/cavosConsole"
alias b="wget -O /dev/null https://ash-speed.hetzner.com/1GB.bin"
alias c="/usr/libexec/webkit2gtk-4.1/MiniBrowser"
alias d="mpv -no-audio --player-operation-mode=pseudo-gui"
