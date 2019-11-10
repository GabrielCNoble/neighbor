#include "matrix.h"
#include <math.h>
#include <stdint.h>


void mat4_t_identity(mat4_t *m)
{
    m->mcomps[0][0] = 1.0;
    m->mcomps[0][1] = 0.0;
    m->mcomps[0][2] = 0.0;
    m->mcomps[0][3] = 0.0;

    m->mcomps[1][0] = 0.0;
    m->mcomps[1][1] = 1.0;
    m->mcomps[1][2] = 0.0;
    m->mcomps[1][3] = 0.0;

    m->mcomps[2][0] = 0.0;
    m->mcomps[2][1] = 0.0;
    m->mcomps[2][2] = 1.0;
    m->mcomps[2][3] = 0.0;

    m->mcomps[3][0] = 0.0;
    m->mcomps[3][1] = 0.0;
    m->mcomps[3][2] = 0.0;
    m->mcomps[3][3] = 1.0;
}

void mat4_t_mul(mat4_t *result, mat4_t *a, mat4_t *b)
{
    /* TODO: SSE */

    for(uint32_t i = 0; i < 4; i++)
    {
        result->mcomps[i][0] = a->mcomps[i][0] * b->mcomps[0][0] + 
                               a->mcomps[i][1] * b->mcomps[1][0] +
                               a->mcomps[i][2] * b->mcomps[2][0] + 
                               a->mcomps[i][3] * b->mcomps[3][0];

        result->mcomps[i][1] = a->mcomps[i][0] * b->mcomps[0][1] + 
                               a->mcomps[i][1] * b->mcomps[1][1] +
                               a->mcomps[i][2] * b->mcomps[2][1] + 
                               a->mcomps[i][3] * b->mcomps[3][1];

        result->mcomps[i][2] = a->mcomps[i][0] * b->mcomps[0][2] + 
                               a->mcomps[i][1] * b->mcomps[1][2] +
                               a->mcomps[i][2] * b->mcomps[2][2] + 
                               a->mcomps[i][3] * b->mcomps[3][2];

        result->mcomps[i][3] = a->mcomps[i][0] * b->mcomps[0][3] + 
                               a->mcomps[i][1] * b->mcomps[1][3] +
                               a->mcomps[i][2] * b->mcomps[2][3] + 
                               a->mcomps[i][3] * b->mcomps[3][3];
    } 
}

void mat4_t_transpose(mat4_t *m)
{
    float t;

    t = m->mcomps[1][0];
    m->mcomps[1][0] = m->mcomps[0][1];
    m->mcomps[0][1] = t;

    t = m->mcomps[2][0];
    m->mcomps[2][0] = m->mcomps[0][2];
    m->mcomps[0][2] = t;

    t = m->mcomps[3][0];
    m->mcomps[3][0] = m->mcomps[0][3];
    m->mcomps[0][3] = t;

    t = m->mcomps[3][1];
    m->mcomps[3][1] = m->mcomps[1][3];
    m->mcomps[1][3] = t;

    t = m->mcomps[3][2];
    m->mcomps[3][2] = m->mcomps[2][3];
    m->mcomps[2][3] = t;

    t = m->mcomps[2][1];
    m->mcomps[2][1] = m->mcomps[1][2];
    m->mcomps[1][2] = t;
}

void mat4_t_persp(mat4_t *m, float fov_y, float aspect, float z_near, float z_far)
{
    float t = tanf(fov_y) * z_near;
    float r = t * aspect;

    mat4_t_identity(m);

    m->mcomps[0][0] = z_near / r;
    m->mcomps[1][1] = z_near / t;
    m->mcomps[2][2] = -1.0;
    m->mcomps[2][3] = -1.0;
    m->mcomps[3][2] = -(2.0 * z_near * z_far) / (z_far - z_near);
    m->mcomps[3][3] = 0.0;
}

void mat4_t_pitch(mat4_t *m, float pitch)
{
    mat4_t_identity(m);
    float s = sinf(pitch * 3.14159265);
    float c = cosf(pitch * 3.14159265);

    m->vcomps[1].comps[1] = c;
    m->vcomps[1].comps[2] = s;
    m->vcomps[2].comps[1] = -s;
    m->vcomps[2].comps[2] = c;
}

void mat4_t_yaw(mat4_t *m, float yaw)
{
    mat4_t_identity(m);

    float s = sinf(yaw * 3.14159265);
    float c = cosf(yaw * 3.14159265);

    m->vcomps[0].comps[0] = c;
    m->vcomps[0].comps[2] = -s;
    m->vcomps[2].comps[0] = s;
    m->vcomps[2].comps[2] = c;
}

void mat4_t_invvm(mat4_t *m)
{
    /* TODO: SSE */
    mat4_t_transpose(m);

    m->vcomps[3].comps[0] = -m->mcomps[0][3] * m->mcomps[0][0]
                            -m->mcomps[1][3] * m->mcomps[1][0]
                            -m->mcomps[2][3] * m->mcomps[2][0];
    
    m->vcomps[3].comps[1] = -m->mcomps[0][3] * m->mcomps[0][1]
                            -m->mcomps[1][3] * m->mcomps[1][1]
                            -m->mcomps[2][3] * m->mcomps[2][1];

    m->vcomps[3].comps[2] = -m->mcomps[0][3] * m->mcomps[0][2]
                            -m->mcomps[1][3] * m->mcomps[1][2]
                            -m->mcomps[2][3] * m->mcomps[2][2];

    m->mcomps[0][3] = 0.0;
    m->mcomps[1][3] = 0.0;
    m->mcomps[2][3] = 0.0;
}