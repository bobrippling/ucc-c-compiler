manual: manual.o
	ld -o $@ $<

manual.o: manual.i
	as -o $@ $<

manual.i: manual.s
	cpp $< > $@

clean:
	rm -f manual manual.[oi]

dump: manual
	objdump --dwarf $<

.PHONY: clean dump
