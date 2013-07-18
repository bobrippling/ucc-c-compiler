// RUN: %ucc -D'yo yo' -E %s | grep '^yo 1$'
// RUN: %ucc -D'yo=yo' -E %s | grep '^yo$'

yo
