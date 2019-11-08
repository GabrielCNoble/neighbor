#include "vector.h"

void vec4_t_add(vec4_t *r, vec4_t *a, vec4_t *b)
{
    r->comps[0] = a->comps[0] + b->comps[0];
    r->comps[1] = a->comps[1] + b->comps[1];
    r->comps[2] = a->comps[2] + b->comps[2];
    r->comps[3] = a->comps[3] + b->comps[3];
}

void vec4_t_sub(vec4_t *r, vec4_t *a, vec4_t *b)
{
    r->comps[0] = a->comps[0] - b->comps[0];
    r->comps[1] = a->comps[1] - b->comps[1];
    r->comps[2] = a->comps[2] - b->comps[2];
    r->comps[3] = a->comps[3] - b->comps[3];
}

void vec4_t_mul(vec4_t *r, float m)
{
    r->comps[0] *= m;
    r->comps[1] *= m;
    r->comps[2] *= m;
    r->comps[3] *= m;
}