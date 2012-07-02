DIRS = assignment/ blocks/ decl_parsing/ inline/ lib/ structs/ type_parsing/

all: ${DIRS:/=/test}
clean: ${DIRS:/=/clean}

%/test:
	make -C$* test

%/clean:
	make -C$* clean
