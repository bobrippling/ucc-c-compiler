// RUN: %ucc -g -S -o- %s | grep -F .loc | cut -d' ' -f3 | grep '1[210]' | %output_check 12 11 10

int f(int);
int g(int);
int h(void);

main()
{
	return
		f(
			g(
				h()));
}
