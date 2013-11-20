// RUN: %layout_check %s

typedef struct
{
	unsigned hex;
	int alpha;
} Color;

typedef struct
{
	int location;
	Color colour;
} ColLoc;
#define ColLocLast -sizeof 5

typedef struct
{
	int width;
	Color colour;
} hline;

enum { A, B, C };

const struct
{
	Color bg,
				sel,
				bg2,
				bg3,
				bg4,
				bg5,
				bg6,
				fg1,
				fg2;

	ColLoc *grad1,
				 *grad2,
				 *grad3,
				 *grad4;

	hline hlines[4];

} _defaultColours[] = {
	[A] = {
		.bg = { 0xFFFFFF, 10 },
		.sel = { 0x0274ee, 10 },
		.bg2 = { 0x000000, -10 },
		.bg3 = { 0xFFFFFF, 10 },
		.bg4 = { 0x000000 , 8 },
		.bg5 = { 0x717880, 10 },
		.bg6 = { 0xffffff, 10 },
		.fg1 = { 0x000000, 10 },
		.fg2 = { 0xffffff, 10 },

		.hlines = {
			{ 1, { 0xFFFFFF, 30 }},
			{ 1, { 0x000000, 22 }},
			{ 1, { 0xFFFFFF, 30 }},
			{ 1, { 0x000000, 22 }},
		},

		.grad1 = (ColLoc[]){
			{ 0, 0, ColLocLast }
		},

		.grad2 = (ColLoc[]){
			{ 1 - 100, 0xFFFFFF, 00 },
			{ 1 - 10, 0x000000, 04 },
			{ 1 - 00, 0x000000, 8 },
			{ 0, 0, ColLocLast }
		},

		.grad3 = (ColLoc[]){
			{ 1 - 100, 0xf457, 100 },
			{ 1 - 68, 0xebecef, 100 },
			{ 1 - 28, 0xc4c7d0, 100 },
			{ 1 - 00, 0xa8acb9, 100 },
			{ 0, 0, ColLocLast }
		},

		.grad4 = (ColLoc[]){
			{ 1 - 100, 0xa3afb8, 80 },
			{ 1 - 66, 0xa3b0b8, 80 },
			{ 1 - 33, 0x93a1ad, 80 },
			{ 1 - 00, 0x8294a1, 80 },
			{ 0, 0, ColLocLast }
		},
	},

	[B] = {
		.bg = { 0xF8BFF, 10 },
		.sel = { 0x3ba1ad, 10 },
		.bg2 = { 0xDEE4EB, 10 },
		.bg3 = { 0x9AB1CC, 10 },
		.bg4 = { 0x89cab, 10 },
		.bg5 = { 0xf8bff, 10 },
		.bg6 = { 0xf8bff, 10 },
		.fg1 = { 0x4c545, 10 },
		.fg2 = { 0xf8bff, 10 },

		.hlines = {
			{ 1, { 0xFFFFFF, 60 }},
			{ 1, { 0x000000, 22 }},
			{ 1, { 0xFFFFFF, 60 }},
			{ 1, { 0x000000, 22 }},
		},

		.grad1 = (ColLoc[]){
			{ 1 - 10, 0xFFFFFF, 00 },
			{ 1 - 1, 0x000000, 04 },
			{ 1 - 0, 0x000000, 8 },
			{ 0, 0, ColLocLast }
		},

		.grad2 = (ColLoc[]){
			{ 1 - 10, 0x058cf5, 0 },
			{ 1 - 0, 0x015de6, 0 },
			{ 0, 0, ColLocLast }
		},

		.grad3 = (ColLoc[]){
			{ 1 - 100, 0xFFFFFF, 00 },
			{ 1 - 98, 0xFFFFFF, 18 },
			{ 1 - 51, 0x000000, 12 },
			{ 1 - 48, 0x000000, 18 },
			{ 1 - 00, 0x000000, 40 },
			{ 0, 0, ColLocLast }
		},

		.grad4 = (ColLoc[]){
			{ 1 - 10, 0xFFFFFF, 20 },
			{ 1 - 9, 0xFFFFFF, 16 },
			{ 1 - 0, 0x000000, 20 },
			{ 0, 0, ColLocLast }
		},
	},

	[C] = {
		.bg = { 0x4D5467, 10 },
		.sel = { 0x945db7, 10 },
		.bg2 = { 0x32394A, 10 },
		.bg3 = { 0xE3D158, 10 },
		.bg4 = { 0x242a39, 10 },
		.bg5 = { 0xf8bff, 10 },
		.bg6 = { 0xf8bff, 10 },
		.fg1 = { 0xc4ccda, 10 },
		.fg2 = { 0xc4ccda, 10 },

		.hlines = {
			{ 1, { 0xFFFFFF, 30 }},
			{ 1, { 0x000000, 22 }},
			{ 1, { 0xFFFFFF, 30 }},
			{ 1, { 0x000000, 22 }},
		},

		.grad1 = (ColLoc[]){
			{ 1 - 10, 0xFFFFFF, 00 },
			{ 1 - 1, 0x000000, 04 },
			{ 1 - 0, 0x000000, 8 },
			{ 0, 0, ColLocLast }
		},

		.grad2 = (ColLoc[]){
			{ 1 - 10, 0x058cf5, 0 },
			{ 1 - 0, 0x015de6, 0 },
			{ 0, 0, ColLocLast }
		},

		.grad3 = (ColLoc[]){
			{ 1 - 100, 0xFFFFFF, 00 },
			{ 1 - 98, 0xFFFFFF, 18 },
			{ 1 - 51, 0x000000, 12 },
			{ 1 - 48, 0x000000, 18 },
			{ 1 - 00, 0x000000, 40 },
			{ 0, 0, ColLocLast }
		},

		.grad4 = (ColLoc[]){
			{ 1 - 10, 0xFFFFFF, 20 },
			{ 1 - 9, 0xFFFFFF, 16 },
			{ 1 - 0, 0x000000, 20 },
			{ 0, 0, ColLocLast }
		},
	},
};
