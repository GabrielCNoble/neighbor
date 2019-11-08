#ifndef MATRIX_H
#define MATRIX_H

#include "vector.h"

typedef union
{
    vec4_t vcomps[4];
    float mcomps[4][4];
}mat4_t;

void mat4_t_identity(mat4_t *m);

void mat4_t_mul(mat4_t *result, mat4_t *a, mat4_t *b);

void mat4_t_transpose(mat4_t *m);

void mat4_t_persp(mat4_t *m ,float fov_y, float aspect, float z_near, float z_far);

void mat4_t_pitch(mat4_t *m, float pitch);

void mat4_t_yaw(mat4_t *m, float yaw);

void mat4_t_invvm(mat4_t *m);


#endif