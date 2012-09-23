typedef int int32_t __attribute__ ((__mode__ (__SI__)));
typedef int32_t mksh_ari_t;
struct ctasserts
{
	char ari_is_signed[((mksh_ari_t)-1 < (mksh_ari_t)0) ? 1 : -1];

	char ari_sign_32_bit_and_wrap[
		(
		 (mksh_ari_t)(((((mksh_ari_t)1 << 15) << 15) - 1) * 2 + 1)
			>
		 (mksh_ari_t)(((((mksh_ari_t)1 << 15) << 15) - 1) * 2 + 2)
		) ? 1 : -1];
};

int main(void)
{
	return sizeof(struct ctasserts); // 2
}
