// RUN: %check -e %s

#if hi // CHECK: note: to match this

#if yo

#endif


end of file // CHECK: error: endif expected
