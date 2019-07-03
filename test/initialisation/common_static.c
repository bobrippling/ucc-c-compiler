// RUN: %ucc -S -o- %s -target x86_64-linux  | grep -F '.local staticint'
// RUN: %ucc -S -o- %s -target x86_64-linux  | grep -F '.comm staticint,4,4'
// RUN: %ucc -S -o- %s -target x86_64-darwin | grep -F 'staticint:'

static int staticint;
