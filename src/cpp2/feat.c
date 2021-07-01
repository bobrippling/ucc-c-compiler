#include "../util/platform.h"

#include "feat.h"

int ucc_has_threads(void)
{
	return platform_supports_threads();
}
