#ifndef SANITIZE_H
#define SANITIZE_H

void sanitize_boundscheck(
		expr *elhs, expr *erhs,
		out_ctx *octx,
		const out_val *lhs, const out_val *rhs);

void sanitize_vlacheck(const out_val *vla_sz, out_ctx *);

void sanitize_shift(
		expr *elhs, expr *erhs,
		enum op_type,
		out_ctx *octx,
		const out_val *lhs, const out_val *rhs);

#endif
