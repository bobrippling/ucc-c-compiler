// RUN: %ocheck 5 %s


main()
{
        __asm__(
                "movl $5, %eax\n"
                "mov $5, %edi\n"
                "syscall\n"
        );
        return 7;
}
