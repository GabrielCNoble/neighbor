#include "vector.h"
#include <math.h>

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

float vec4_t_dot(vec4_t *a, vec4_t *b)
{
    return a->comps[0] * b->comps[0] + 
           a->comps[1] * b->comps[1] +
           a->comps[2] * b->comps[2] +
           a->comps[3] * b->comps[3];
}

float vec4_t_len(vec4_t *v)
{
    return sqrt(v->comps[0] * v->comps[0] + 
                v->comps[1] * v->comps[1] +
                v->comps[2] * v->comps[2] +
                v->comps[3] * v->comps[3]);
}

void vec4_t_normalize(vec4_t *v)
{
    float l = vec4_t_len(v);

    if(l)
    {
        v->comps[0] /= l;
        v->comps[1] /= l;
        v->comps[2] /= l;
        v->comps[3] /= l;
    }
}




void vec3_t_add(vec3_t *r, vec3_t *a, vec3_t *b)
{
    r->comps[0] = a->comps[0] + b->comps[0];
    r->comps[1] = a->comps[1] + b->comps[1];
    r->comps[2] = a->comps[2] + b->comps[2];
}

void vec3_t_sub(vec3_t *r, vec3_t *a, vec3_t *b)
{
    r->comps[0] = a->comps[0] - b->comps[0];
    r->comps[1] = a->comps[1] - b->comps[1];
    r->comps[2] = a->comps[2] - b->comps[2];
}

void vec3_t_mul(vec3_t *r, float m)
{
    r->comps[0] *= m;
    r->comps[1] *= m;
    r->comps[2] *= m;
}

float vec3_t_dot(vec3_t *a, vec3_t *b)
{
    return a->comps[0] * b->comps[0] + 
           a->comps[1] * b->comps[1] +
           a->comps[2] * b->comps[2];
}

void vec3_t_cross(vec3_t *r, vec3_t *a, vec3_t *b)
{

}

float vec3_t_len(vec3_t *v)
{
    return sqrt(v->comps[0] * v->comps[0] + 
                v->comps[1] * v->comps[1] +
                v->comps[2] * v->comps[2]);
}

void vec3_t_normalize(vec3_t *v)
{
    float l = vec3_t_len(v);

    if(l)
    {
        v->comps[0] /= l;
        v->comps[1] /= l;
        v->comps[2] /= l;
    }
}