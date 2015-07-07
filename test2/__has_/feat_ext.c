// RUN: %ocheck 0 %s -std=c99 -DEXPECTED_FEAT_ALIGNAS=0
// RUN: %ocheck 0 %s -std=c11 -DEXPECTED_FEAT_ALIGNAS=1

int feat_alignas = __has_feature(c_alignas);
int ext_alignas = __has_extension(c_alignas);

main()
{
	if(feat_alignas != EXPECTED_FEAT_ALIGNAS)
		abort();

	if(!ext_alignas)
		abort();

	return 0;
}
