// RUN: %ucc -S -o- %s | grep 'movzlq'; [ $? -ne 0 ]
struct timespec
{
	long tv_sec, tv_nsec;
};

zlq(unsigned int sec)
{
	struct timespec tsp;
	tsp.tv_sec = sec;
}
