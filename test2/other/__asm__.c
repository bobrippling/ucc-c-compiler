// RUN: %ucc -c %s
// RUN_: %ucc -DEXIT_CODE='???' -o %t %s
// RUN_: %t; [ $? -eq 5 ]


main()
{
        __asm__(
                "movl $" EXIT_CODE ", %eax\n"
                "mov $5, %edi\n"
                "syscall\n"
        );
        return 7;
}
