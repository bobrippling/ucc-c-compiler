#ifndef FORWARDS_H
#define FORWARDS_H

typedef struct out_ctx out_ctx;
typedef struct out_blk out_blk;
typedef struct out_val out_val;

enum out_pic_type
{
	OUT_LBL_NOPIC = 0,
	OUT_LBL_PIC = 1 << 0,
	OUT_LBL_PICLOCAL = 1 << 1,
};

#endif
