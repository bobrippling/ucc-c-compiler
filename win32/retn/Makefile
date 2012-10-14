WIN32 = /cygdrive/c/Windows/System32

LIBS = ${WIN32}/user32.dll ${WIN32}/kernel32.dll

retn: retn.o
	ld -o $@ $< ${LIBS}
