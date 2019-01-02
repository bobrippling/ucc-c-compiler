// RUN: %ucc -g -S -o- %s | grep -F .loc | cut -d' ' -f3 | grep '[876]' | %output_check 8 7 6

main()
{
	return
		f(
			g(
				h()));
}
