// RUN: %ucc -o %t %s -fsigned-char      && %ocheck 1 %t
// RUN: %ucc -o %t %s -fno-signed-char   && %ocheck 0 %t
// RUN: %ucc -o %t %s -fno-unsigned-char && %ocheck 1 %t
// RUN: %ucc -o %t %s -funsigned-char    && %ocheck 0 %t

// v- signed by default
// RUN: %ocheck 1 %s

main()
{
	return __builtin_is_signed(char);
}
