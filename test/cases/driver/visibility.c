// RUN: %ucc -x cpp-output %s -fsyntax-only -fvisibility=protected -target x86_64-linux
// RUN: %ucc -x cpp-output %s -fsyntax-only -fvisibility=protected -target x86_64-darwin 2>&1 | grep 'unsupported visibility'
