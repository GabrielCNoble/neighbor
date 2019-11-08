#ifndef VECTOR_H
#define VECTOR_H

typedef struct
{
    float comps[4];
}vec4_t;

#define vec4_t_zero (vec4_t){0.0, 0.0, 0.0, 0.0}

void vec4_t_add(vec4_t *r, vec4_t *a, vec4_t *b);

void vec4_t_sub(vec4_t *r, vec4_t *a, vec4_t *b);

void vec4_t_mul(vec4_t *r, float m);

#endif