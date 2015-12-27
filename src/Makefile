MAKE_DIR = make -C $@

all: cc1 cpp2 ucc

ucc: util
	${MAKE_DIR}

cc1: util strbuf
	${MAKE_DIR}

cpp2: util
	${MAKE_DIR}

util:
	${MAKE_DIR}

strbuf:
	${MAKE_DIR}

deps:
	make -BCutil deps
	make -BCcpp2 deps
	make -BCcc1 deps
	make -BCucc deps

clean:
	make -C cc1  clean
	make -C cpp2 clean
	make -C ucc  clean
	make -C util clean
	make -C strbuf clean

.PHONY: cc1 cpp2 ucc util strbuf clean
