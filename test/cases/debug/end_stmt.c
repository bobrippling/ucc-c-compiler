// RUN: %debug_check %s

a() {
	int v = 0;
	for(;;){
		v++;
	} // we should emit a location for the close brace (jump-to-body)
}

b() {
	int v;
	for(v = 0; v < 20; v++){
		;
	} // we should emit a location for the close brace (jump-to-condition)
}
