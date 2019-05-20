// RUN: %ucc -S -o %t %s -fno-pie -target x86_64-cygwin
// RUN: ! awk '/^_nocheck/  {p=1}/Lfuncend/{p=0}{if(p)print}' < %t | grep chkstk
// RUN: ! awk '/^_nocheck/  {p=1}/Lfuncend/{p=0}{if(p)print}' < %t | grep alloca
// RUN:   awk '/^_yescheck/ {p=1}/Lfuncend/{p=0}{if(p)print}' < %t | grep chkstk
// RUN: ! awk '/^_yescheck/ {p=1}/Lfuncend/{p=0}{if(p)print}' < %t | grep alloca
//
// RUN: %ucc -S -o %t %s -fno-pie -target i386-cygwin
// RUN: ! awk '/^_nocheck/  {p=1}/Lfuncend/{p=0}{if(p)print}' < %t | grep chkstk
// RUN: ! awk '/^_nocheck/  {p=1}/Lfuncend/{p=0}{if(p)print}' < %t | grep alloca
// RUN: ! awk '/^_yescheck/ {p=1}/Lfuncend/{p=0}{if(p)print}' < %t | grep chkstk
// RUN:   awk '/^_yescheck/ {p=1}/Lfuncend/{p=0}{if(p)print}' < %t | grep alloca

// FIXME: replace these awk commands with stdoutcheck from `fix/make-test`

enum { PAGESIZE = 4096 };

void nocheck(void)
{
}

void yescheck(void)
{
	char buf[PAGESIZE];
}
