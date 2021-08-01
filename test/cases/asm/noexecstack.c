// RUN: %ucc -S -o- %s -target x86_64-linux | grep -Fi '.note.GNU-stack'
main()
{
}

// should get a .GNU-NOTE.stack exec section
