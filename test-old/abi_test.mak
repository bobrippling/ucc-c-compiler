# this is to help for debugging inter-compiler ABI bugs

SRC = SOURCE_GOES_HERE
LD = cc

.PHONY: all clean

all: impl_tcc.main_tcc impl_tcc.main_gcc impl_tcc.main_ucc impl_tcc.main_clang \
	impl_gcc.main_tcc impl_gcc.main_gcc impl_gcc.main_ucc impl_gcc.main_clang    \
	impl_ucc.main_tcc impl_ucc.main_gcc impl_ucc.main_ucc impl_ucc.main_clang    \
	impl_clang.main_tcc impl_clang.main_gcc impl_clang.main_ucc                  \
	impl_clang.main_clang

impl_tcc.main_tcc: impl_tcc.o main_tcc.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_tcc.main_gcc: impl_tcc.o main_gcc.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_tcc.main_ucc: impl_tcc.o main_ucc.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_tcc.main_clang: impl_tcc.o main_clang.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_gcc.main_tcc: impl_gcc.o main_tcc.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_gcc.main_gcc: impl_gcc.o main_gcc.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_gcc.main_ucc: impl_gcc.o main_ucc.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_gcc.main_clang: impl_gcc.o main_clang.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_ucc.main_tcc: impl_ucc.o main_tcc.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_ucc.main_gcc: impl_ucc.o main_gcc.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_ucc.main_ucc: impl_ucc.o main_ucc.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_ucc.main_clang: impl_ucc.o main_clang.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_clang.main_tcc: impl_clang.o main_tcc.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_clang.main_gcc: impl_clang.o main_gcc.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_clang.main_ucc: impl_clang.o main_ucc.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@
impl_clang.main_clang: impl_clang.o main_clang.o
	${LD} -o $@ $^
	echo "$@:"
	@./$@

impl_clang.o: ${SRC}
	clang -DIMPL -c -o $@ $<
main_clang.o: ${SRC}
	clang -c -o $@ $<

impl_gcc.o: ${SRC}
	gcc -DIMPL -c -o $@ $<
main_gcc.o: ${SRC}
	gcc -c -o $@ $<

impl_tcc.o: ${SRC}
	tcc -DIMPL -c -o $@ $<
main_tcc.o: ${SRC}
	tcc -c -o $@ $<

impl_ucc.o: ${SRC}
	../ucc -DIMPL -c -o $@ $<
main_ucc.o: ${SRC}
	../ucc -c -o $@ $<

clean:
	rm -f                   \
		impl_clang.main_clang \
		impl_clang.main_gcc   \
		impl_clang.main_tcc   \
		impl_clang.main_ucc   \
		impl_clang.o          \
		impl_gcc.main_clang   \
		impl_gcc.main_gcc     \
		impl_gcc.main_tcc     \
		impl_gcc.main_ucc     \
		impl_gcc.o            \
		impl_tcc.main_clang   \
		impl_tcc.main_gcc     \
		impl_tcc.main_tcc     \
		impl_tcc.main_ucc     \
		impl_tcc.o            \
		impl_ucc.main_clang   \
		impl_ucc.main_gcc     \
		impl_ucc.main_tcc     \
		impl_ucc.main_ucc     \
		impl_ucc.o            \
		main_clang.o          \
		main_gcc.o            \
		main_tcc.o            \
		main_ucc.o
