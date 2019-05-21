MAKE_DIR = make -C $@

all: cc1 cpp2 ucc rt

ucc: util
	$Q${MAKE_DIR}

cc1: util
	$Q${MAKE_DIR}

cpp2: util
	$Q${MAKE_DIR}

util:
	$Q${MAKE_DIR}

rt: ucc cc1 cpp2 util
	$Q${MAKE_DIR}

clean:
	$Qmake -C cc1  $@
	$Qmake -C cpp2 $@
	$Qmake -C ucc  $@
	$Qmake -C util $@

.PHONY: cc1 cpp2 ucc util rt clean
