CFLAGS = -g -Wall -Wextra -pedantic -std=c99 \
         -Wno-char-subscripts -Wno-format-extra-args \
				 -Wmissing-prototypes -Wno-missing-braces

#-Wconversion          \
#-Wshadow              \
#-Wstrict-prototypes   \

include ../src_config_platform.mk
