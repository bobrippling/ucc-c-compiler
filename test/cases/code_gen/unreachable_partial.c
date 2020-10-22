// RUN: %ocheck 0 %s

_Noreturn void abort(void);

int main(int argc, char *argv[]) {
	char *x = (0 ? (abort(), 0) : 0);

	// 0 ? abort() : xyz()
	// would cause out_ctrl_merge_n() to try to merge the following blocks:
	// [0] = null
	// [1] = xyz()
	//
	// but this meant the array was interpreted as empty since [0] == null,
	// meaning we had a leak or worse from xyz()'s out_val being left lying
	// around.
}
