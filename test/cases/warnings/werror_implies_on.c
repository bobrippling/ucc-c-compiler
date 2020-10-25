// RUN: %check -e %s -Wno-implicit -Werror=implicit

// make sure -Werror=xyz turns on warning 'xyz'

main() // CHECK: error: defaulting type to int
{
}
