// RUN: %ucc -fno-ms-extensions -c %s; [ $? -ne 0 ]
// RUN: %ucc -fms-extensions -c %s

struct ms_extension
{
	union name // anonymous but tagged
	{
		int hex;
	};
} ms;

int *p_ms_ext = &ms.hex;



struct ms_extension2
{
	union name; // anonymous-with-tag
} ms2;

int *p_ms_ext2 = &ms2.hex;
