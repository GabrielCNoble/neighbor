#ifndef MATRIX_H
#define MATRIX_H

#include "vector.h"

typedef union
{
    vec4_t vcomps[4];
    float mcomps[4][4];
}mat4_t;

typedef union
{
    vec3_t vcomps[3];
    float mcomps[3][3];
}mat3_t;


/*
    mat4_t_mul: self-explanatory
*/
void mat4_t_identity(mat4_t *m);

/*
    mat4_t_mul: multiplies two row major matrices. 
*/
void mat4_t_mul(mat4_t *result, mat4_t *a, mat4_t *b);

/*
    mat4_t_transpose: transposes a matrix.
*/
void mat4_t_transpose(mat4_t *m);

/*
    mat4_t_persp: creates a perspective projection matrix.
*/
void mat4_t_persp(mat4_t *m ,float fov_y, float aspect, float z_near, float z_far);

/*
    mat4_t_pitch: creates a pitch matrix. Pitch is not clamped between 1.0 and -1.0.
*/
void mat4_t_pitch(mat4_t *m, float pitch);

/*
    mat4_t_yaw: creates an yaw matrix. Yaw is not clamped between 1.0 and -1.0.
*/
void mat4_t_yaw(mat4_t *m, float yaw);

/*
    mat4_t_invvm: computes the world-to-view matrix from a view-to-world matrix.
*/
void mat4_t_invvm(mat4_t *m);


#endif