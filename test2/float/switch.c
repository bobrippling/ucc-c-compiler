// RUN: %check -e %s

main()
{
	switch(1.0f){ // CHECK: error: switch requires an integral type
	}
}
