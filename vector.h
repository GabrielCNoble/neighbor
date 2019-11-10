#ifndef VECTOR_H
#define VECTOR_H

typedef struct
{
    float comps[4];
}vec4_t;

typedef struct
{
    float comps[3];
}vec3_t;

typedef struct
{
    float comps[2];
}vec2_t;

#define vec4_t_zero (vec4_t){0.0, 0.0, 0.0, 0.0}
#define vec3_t_zero (vec3_t){0.0, 0.0, 0.0}
#define vec2_t_zero (vec2_t){0.0, 0.0}

void vec4_t_add(vec4_t *r, vec4_t *a, vec4_t *b);

void vec4_t_sub(vec4_t *r, vec4_t *a, vec4_t *b);

void vec4_t_mul(vec4_t *r, float m);

float vec4_t_dot(vec4_t *a, vec4_t *b);

float vec4_t_len(vec4_t *v);

void vec4_t_normalize(vec4_t *v);



void vec3_t_add(vec3_t *r, vec3_t *a, vec3_t *b);

void vec3_t_sub(vec3_t *r, vec3_t *a, vec3_t *b);

void vec3_t_mul(vec3_t *r, float m);

float vec3_t_dot(vec3_t *a, vec3_t *b);

void vec3_t_cross(vec3_t *r, vec3_t *a, vec3_t *b);

float vec3_t_len(vec3_t *v);

void vec3_t_normalize(vec3_t *v);

#endif