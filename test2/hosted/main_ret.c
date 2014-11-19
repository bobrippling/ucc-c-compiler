// RUN: %ocheck 0 %s -std=c99
// RUN: %ucc -std=c89 %s -fsyntax-only 2>&1 | grep 'reaches end of'
// RUN: %ucc -std=c99 %s -fsyntax-only 2>&1 | grep 'reaches end of'; [ $? -ne 0 ]

main()
{
}
