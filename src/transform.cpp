

#include "transform.h"
using namespace Tmpl8;

void Transform::Rotate( float3 rotation )
{
	matrix rotationMatrix;
	rotationMatrix.RotateX(m_eulerRotation.x = rotation.x);	
	m_mat.Concatenate( rotationMatrix );
	rotationMatrix.RotateY(m_eulerRotation.y = rotation.y); 
	m_mat.Concatenate( rotationMatrix );
	rotationMatrix.RotateZ(m_eulerRotation.z = rotation.z); 
	m_mat.Concatenate( rotationMatrix );

	forward = float3(m_mat[2], m_mat[6], m_mat[10]);
	up =	float3(m_mat[1], m_mat[5], m_mat[9]);
	right = float3(m_mat[0], m_mat[4], m_mat[8]);
}

void Transform::AxisRotate( float3 axis, float theta )
{
	float length;
	float c,s,t;
	float x = axis.x;
	float y = axis.y;
	float z = axis.z;

	// normalize
	length = sqrt(x*x + y*y + z*z);

	// too close to 0, can't make a normalized vector
	if (length < .000001)
		return;

	x /= length;
	y /= length;
	z /= length;

	// do the trig
	c = cos(theta);
	s = sin(theta);
	t = 1-c;   
   
	matrix rot;
	// build the rotation matrix
	rot[0] = t*x*x + c;
	rot[1] = t*x*y - s*z;
	rot[2] = t*x*z + s*y;
	rot[3] = 0;

	rot[4] = t*x*y + s*z;
	rot[5] = t*y*y + c;
	rot[6] = t*y*z - s*x;
	rot[7] = 0;

	rot[8] = t*x*z - s*y;
	rot[9] = t*y*z + s*x;
	rot[10] = t*z*z + c;
	rot[11] = 0;

	rot.Concatenate(m_mat);
	m_mat = rot;
	forward = float3(m_mat[2], m_mat[6], m_mat[10]);
	up =	float3(m_mat[1], m_mat[5], m_mat[9]);
	right = float3(m_mat[0], m_mat[4], m_mat[8]);
}