#ifndef COMMON_H
#define COMMON_H

#define SNPRINTF(s, n, ...) \
		UCC_ASSERT(snprintf(s, n, __VA_ARGS__) < (signed)n, \
				"snprintf buffer too small")

#endif
