int main()
{
	int i((int(x)));
	//   ^~ this should error (or in C++, initialise `i` with `x`'s value
	// https://github.com/CppCon/CppCon2019/blob/master/Lightning%20Talks%20and%20Lunch%20Sessions/vuint32_t/vuint32_t__matthis_kruse__cppcon_2019.pdf
}
