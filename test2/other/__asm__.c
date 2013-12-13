// RUN: %ucc -c %s
// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 5 ]


main()
{
        __asm__(
                "movl $5, %eax\n"
                "mov $5, %edi\n"
                "syscall\n"
        );
        return 7;
}
