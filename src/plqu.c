/*** plqu.c -- price level queue
 *
 * Copyright (C) 2016-2017 Sebastian Freundt
 *
 * Author:  Sebastian Freundt <freundt@ga-group.nl>
 *
 * This file is part of books.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the author nor the names of any contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **/
#if defined HAVE_CONFIG_H
# include "config.h"
#endif	/* HAVE_CONFIG_H */
#include <stdlib.h>
#include <string.h>
#include "plqu.h"
#include "plqu_val.h"
#include "nifty.h"

#define PLQU_INIZ	(8U)

struct plqu_s {
	size_t z;
	plqu_val_t *a;
	plqu_val_t sum;
	size_t head;
	size_t tail;
};


plqu_t
make_plqu(void)
{
	struct plqu_s *r = malloc(sizeof(*r));
	r->z = PLQU_INIZ;
	r->a = malloc(PLQU_INIZ * sizeof(*r));
	r->sum = plqu_val_0;
	r->head = 0U;
	r->tail = 0U;
	return r;
}

void
free_plqu(plqu_t q)
{
	if (LIKELY(q->a != NULL)) {
		free(q->a);
	}
	free(q);
	return;
}

int
plqu_put(plqu_t q, plqu_qid_t i, plqu_val_t v)
{
	if (UNLIKELY(!(i > q->head && i <= q->tail))) {
		return -1;
	}
	const size_t slot = (i - 1U) % q->z;
	q->sum = plqu_val_sub(q->sum, q->a[slot]);
	q->sum = plqu_val_add(q->sum, q->a[slot] = v);
	return 0;
}

plqu_qid_t
plqu_add(plqu_t q, plqu_val_t v)
{
	if (UNLIKELY(q->tail - q->head >= q->z)) {
		/* resize him */
		const size_t nuz = q->z * 2U;
		void *tmp = realloc(q->a, nuz * sizeof(*q->a));
		if (UNLIKELY(tmp == NULL)) {
			return -1ULL;
		}
		/* otherwise resize */
		q->a = tmp;
		/* and maybe move */
		if (UNLIKELY(!((q->tail - q->head) % q->z))) {
			/* they won't overlap */
			memcpy(q->a + q->z, q->a,
			       (q->tail % q->z) * sizeof(*q->a));
		}
		/* keep track of size */
		q->z = nuz;
	}
	q->a[q->tail++ % q->z] = v;
	q->sum = plqu_val_add(q->sum, v);
	return q->tail;
}

plqu_val_t
plqu_top(plqu_t q)
{
	plqu_val_t r = plqu_val_0;

	if (LIKELY(q->head < q->tail)) {
		r = q->a[q->head % q->z];
	}
	return r;
}

plqu_val_t
plqu_pop(plqu_t q)
{
	plqu_val_t r = plqu_val_0;

	if (LIKELY(q->head < q->tail)) {
		const size_t slot = q->head++ % q->z;
		r = q->a[slot];
		q->sum = plqu_val_sub(q->sum, r);
	}
	return r;
}

plqu_val_t
plqu_sum(plqu_t q)
{
	return q->sum;
}


bool
plqu_iter_next(plqu_iter_t *iter)
{
	if (UNLIKELY(iter->q == NULL)) {
		return false;
	}
	for (size_t i = iter->i; i < iter->q->z; i++) {
		const size_t slot = (iter->q->head + i) % iter->q->z;
		if (LIKELY(!plqu_val_0_p(iter->q->a[slot]))) {
			/* good one */
			iter->v = iter->q->a[slot];
			iter->i = i + 1U;
			return true;
		}
	}
	return false;
}

/* plqu.c ends here */
