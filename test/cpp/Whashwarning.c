// RUN: %check -e %s -Wno-#warning

#warning yo // CHECK: !/warn/

main()
{
}

#error hi // CHECK: error: #error: hi
