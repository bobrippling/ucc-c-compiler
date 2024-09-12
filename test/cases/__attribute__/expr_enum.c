// RUN: %ucc -fsyntax-only %s
enum bool { false, true };

int main() {
   // ensure attr checking on enums doesn't crash
   if(false);
}
