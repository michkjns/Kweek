

#pragma once
#include "template.h"

namespace Tmpl8 {

	struct Transform
	{
		Transform() { m_eulerRotation = float3( 0, 0, 0 ); }
		matrix m_mat;
		float3 m_eulerRotation;

		void Translate( float3 translation ) { m_mat.Translate( translation ); }
		void Rotate( float3 a_rotation );
		void AxisRotate( float3 axis, float theta );
		void SetTranslation( float3 pos ) { m_mat.SetTranslation( pos ); }
		float3 GetTranslation() { return float3( m_mat.cell[3], m_mat.cell[7], m_mat.cell[11] ); }
		float3 up, forward, right;
};


};