# System wide environment variables and startup programs should go into
# /etc/profile.  Personal environment variables and startup programs
# should go into ~/.bash_profile.  Personal aliases and functions should
# go into ~/.bashrc

# Provides colored /bin/ls and /bin/grep commands.  Used in conjunction
# with code in /etc/profile.
source /etc/profile

alias ls='ls --color=auto'
alias grep='grep --color=auto'
alias dir="dir --color=auto"
