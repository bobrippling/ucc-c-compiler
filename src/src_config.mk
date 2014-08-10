CFLAGS = -g -pg -Wall -Wextra -pedantic -std=c99 \
         -Wno-char-subscripts -Wno-format-extra-args \
				 -Wmissing-prototypes -Wno-missing-braces \
				 -pg -O2 -fprofile-arcs -ftest-coverage

LDFLAGS = -lgcov

#-Wconversion          \
#-Wshadow              \
#-Wstrict-prototypes   \

include ../src_config_platform.mk
