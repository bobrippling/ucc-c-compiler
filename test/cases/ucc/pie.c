// RUN: ! %ucc -target x86_64-linux -E -xc /dev/null -dM       | grep 'pi[ce]'
//
// RUN:   %ucc -target x86_64-linux -E -xc /dev/null -dM -fpic | grep ' __pic__ 1'
// RUN: ! %ucc -target x86_64-linux -E -xc /dev/null -dM -fpic | grep 'pie'
// RUN:   %ucc -target x86_64-linux -E -xc /dev/null -dM -fpie | grep ' __pic__ 1'
// RUN:   %ucc -target x86_64-linux -E -xc /dev/null -dM -fpie | grep ' __pie__ 1'
//
// RUN:   %ucc -target x86_64-linux -E -xc /dev/null -dM -fPIC | grep ' __pic__ 2'
// RUN: ! %ucc -target x86_64-linux -E -xc /dev/null -dM -fPIC | grep 'pie'
// RUN:   %ucc -target x86_64-linux -E -xc /dev/null -dM -fPIE | grep ' __pic__ 2'
// RUN:   %ucc -target x86_64-linux -E -xc /dev/null -dM -fPIE | grep ' __pie__ 2'
//
// RUN: ! %ucc -target x86_64-linux -E -xc /dev/null -dM -fpie -fno-pie | grep 'pie'
// RUN: ! %ucc -target x86_64-linux -E -xc /dev/null -dM -fpie -fno-pic | grep 'pie'
// RUN: ! %ucc -target x86_64-linux -E -xc /dev/null -dM -fpie -fno-pic | grep 'pic'
// RUN:   %ucc -target x86_64-linux -E -xc /dev/null -dM -fno-pie -fpie | grep ' __pie__ 1'
// RUN:   %ucc -target x86_64-linux -E -xc /dev/null -dM -fno-pie -fpie | grep ' __pic__ 1'
// RUN:   %ucc -target x86_64-linux -E -xc /dev/null -dM -fno-pie -fpic | grep ' __pic__ 1'
// RUN: ! %ucc -target x86_64-linux -E -xc /dev/null -dM -fno-pie -fpic | grep 'pie'
