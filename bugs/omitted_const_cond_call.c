int x = f() && 0;

f(int i)
{
	// have ->f_side_effects() and check it when doing constant jump conditions?
	// then it can also be checked in places like desig init. overwrite
	if(i && h() && 0){
		g();
	}
}
