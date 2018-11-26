// TEST: target linux
// RUN: %ucc -S -o- %s | grep -Fi '.note.GNU-stack'
main()
{
}

// should get a .GNU-NOTE.stack exec section
