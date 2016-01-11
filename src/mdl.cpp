/*
* mdl.c -- mdl model loader
* last modification: aug. 14, 2007
*
* Copyright (c) 2005-2007 David HENRY
*
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without
* restriction, including without limitation the rights to use,
* copy, modify, merge, publish, distribute, sublicense, and/or
* sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
* ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
* CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
* WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* gcc -Wall -ansi -lGL -lGLU -lglut mdl.c -o mdl
*/

#include "mdl.h"
#include "renderer.h"
#include "surface.h"
#include "scene.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace Tmpl8;
mdl_model_t mdlfile;




/**
* Free resources allocated for the model.
*/
void FreeModel( struct mdl_model_t *mdl )
{
	int i;

	if( mdl->skins )
	{
		free( mdl->skins );
		mdl->skins = NULL;
	}

	if( mdl->texcoords )
	{
		free( mdl->texcoords );
		mdl->texcoords = NULL;
	}

	if( mdl->triangles )
	{
		free( mdl->triangles );
		mdl->triangles = NULL;
	}

	if( mdl->tex_id )
	{
		/* Delete OpenGL textures */
		//  glDeleteTextures (mdl->header.num_skins, mdl->tex_id);

		free( mdl->tex_id );
		mdl->tex_id = NULL;
	}

	if( mdl->frames )
	{
		for( i = 0; i < mdl->header.num_frames; ++i )
		{
			free( mdl->frames[i].frame.verts );
			mdl->frames[i].frame.verts = NULL;
		}

		free( mdl->frames );
		mdl->frames = NULL;
	}
}
/**
* Make a texture given a skin index 'n'.
*/
GLuint MakeTextureFromSkin( int n, const struct mdl_model_t *mdl )
{
	int i;
	GLuint id;

	GLubyte *pixels = (GLubyte *)
		malloc( mdl->header.skinwidth * mdl->header.skinheight * 3 );

	/* Convert indexed 8 bits texture to RGB 24 bits */
	for( i = 0; i < mdl->header.skinwidth * mdl->header.skinheight; ++i )
	{
		pixels[( i * 3 ) + 0] = colormap[mdl->skins[n].data[i]][0];
		pixels[( i * 3 ) + 1] = colormap[mdl->skins[n].data[i]][1];
		pixels[( i * 3 ) + 2] = colormap[mdl->skins[n].data[i]][2];
	}

	Surface* texture = new Surface( mdl->header.skinwidth, mdl->header.skinheight );
	for( int i = 0; i < texture->GetWidth() * texture->GetHeight(); i++ )
	{
		unsigned char oc = pixels[i * 3];
		int r = pixels[( i * 3 ) + 0];
		GLubyte g = pixels[( i * 3 ) + 1];
		GLubyte b = pixels[( i * 3 ) + 2];
		texture->GetBuffer()[i] = ( r << 16 ) + ( g << 8 ) + b;
	}

	id = MaterialMgr::Get()->AddMat( "mdl", texture );
	free( pixels );
	return id;
}
Mesh* MDL_to_Mesh( mdl_model_t* mdl, float3 a_Offset, float a_Scale )
{
	Mesh* mesh = new Mesh();
	mesh->m_subCount = 1;
	mesh->m_subMeshes = new SubMesh[1];
	SubMesh* sm = &mesh->m_subMeshes[0];
	mesh->m_vertSize = mdl->header.num_verts;
	mesh->m_vertData = new Vertex*[mdl->header.num_frames];
	mesh->m_vertDataTransf = new Vertex[mdl->header.num_verts];
	mesh->m_frames = mdl->header.num_frames;

	matrix m, n;
	m.RotateX( 90 );
	n.RotateY( 90 );
//	m.Concatenate( n );
	for( int i = 0; i < mdl->header.num_frames; i++ )
	{
		mesh->m_vertData[i] = new Vertex[mesh->m_vertSize];
	}
	for( unsigned int j = 0; j < mesh->m_frames; j++ )	for( unsigned int i = 0; i < mesh->m_vertSize; i++ )
	{
		mesh->m_vertData[j][i].pos = float3(
			mdl->frames[j].frame.verts[i].v[0],
			mdl->frames[j].frame.verts[i].v[1],
			mdl->frames[j].frame.verts[i].v[2] ) * float3( mdl->header.scale[0],
			mdl->header.scale[1], mdl->header.scale[2] )  * 2.0f * a_Scale;

		mesh->m_vertData[j][i].pos += float3(
			mdl->header.translate[0],
			mdl->header.translate[1],
			mdl->header.translate[2]
			) * 2;

		mesh->m_vertData[j][i].pos = n.Transform( m.Transform( mesh->m_vertData[j][i].pos ) * 0.05f + a_Offset );

		mesh->m_vertData[j][i].uv.x = ( mdl->texcoords[i].s + 0.5f ) / mdl->header.skinwidth;
		mesh->m_vertData[j][i].uv.y = 1 - ( mdl->texcoords[i].t + 0.5f ) / mdl->header.skinheight;
		mesh->m_vertData[j][i].onseam = mdl->texcoords[i].onseam;
		if( mdl->texcoords[i].onseam > 0 )
			int a = 1;
	}
	sm->m_triSize = mdl->header.num_tris;
	sm->m_triangles = new Tri[sm->m_triSize];
	for( unsigned int i = 0; i < sm->m_triSize; i++ )
	{
		Tri* t = &sm->m_triangles[i];
		t->m_vertices[0] = mdl->triangles[i].vertex[0];
		t->m_vertices[1] = mdl->triangles[i].vertex[1];
		t->m_vertices[2] = mdl->triangles[i].vertex[2];
		t->m_vertices[3] = mdl->triangles[i].facesfront;
		//if( mdl->triangles[i].facesfront )
		//{
		//	for( unsigned int j = 0; j < mesh->m_frames; j++ )
		//	{
		//		if( mdl->texcoords[t->m_vertices[0]].onseam )
		//		{
		//			mesh->m_vertData[j][mdl->triangles[i].vertex[0] ].uv.x += 0.5f;
		//		}
		//		if( mdl->texcoords[t->m_vertices[1]].onseam )
		//		{
		//			mesh->m_vertData[j][mdl->triangles[i].vertex[1]].uv.x += 0.5f;
		//		}
		//		if( mdl->texcoords[t->m_vertices[2]].onseam )
		//		{
		//			mesh->m_vertData[j][mdl->triangles[i].vertex[2]].uv.x += 0.5f;
		//		}
		//	}
		//}
	}
	sm->m_material = mdl->tex_id[0];

	FreeModel( mdl );
	delete mdl;
	Renderer::Instance->m_scene->m_meshlist.push_back( mesh );
	mesh->GenerateAABB();
	return mesh;

}

/**
* Load an MDL model from file.
*
* Note: MDL format stores model's data in little-endian ordering.  On
* big-endian machines, you'll have to perform proper conversions.
*/
Mesh* Renderer::LoadMDL( const char *filename, float3 a_Offset, float a_Scale )
{
	FILE *fp;
	int i;
	mdl_model_t *mdl = new mdl_model_t();
	fp = fopen( filename, "rb" );
	if( !fp )
	{
		fprintf( stderr, "error: couldn't open \"%s\"!\n", filename );
		return 0;
	}

	/* Read header */
	fread( &mdl->header, 1, sizeof ( mdl_header_t ), fp );

	if( ( mdl->header.ident != 1330660425 ) ||
		( mdl->header.version != 6 ) )
	{
		/* Error! */
		fprintf( stderr, "Error: bad version or identifier\n" );
		fclose( fp );
		return 0;
	}

	/* Memory allocations */
	mdl->skins = ( struct mdl_skin_t * )
		malloc( sizeof ( struct mdl_skin_t ) * mdl->header.num_skins );
	mdl->texcoords = ( struct mdl_texcoord_t * )
		malloc( sizeof ( struct mdl_texcoord_t ) * mdl->header.num_verts );
	mdl->triangles = ( struct mdl_triangle_t * )
		malloc( sizeof ( struct mdl_triangle_t ) * mdl->header.num_tris );
	mdl->frames = ( struct mdl_frame_t * )
		malloc( sizeof ( struct mdl_frame_t ) * mdl->header.num_frames );
	mdl->tex_id = (GLuint *)
		malloc( sizeof (GLuint)* mdl->header.num_skins );

	mdl->iskin = 0;

	/* Read texture data */
	for( i = 0; i < mdl->header.num_skins; ++i )
	{
		mdl->skins[i].data = (GLubyte *)malloc( sizeof (GLubyte)
			* mdl->header.skinwidth * mdl->header.skinheight );

		fread( &mdl->skins[i].group, sizeof ( int ), 1, fp );
		fread( mdl->skins[i].data, sizeof ( GLubyte ),
			mdl->header.skinwidth * mdl->header.skinheight, fp );

		mdl->tex_id[i] = MakeTextureFromSkin( i, mdl );

		free( mdl->skins[i].data );
		mdl->skins[i].data = NULL;
	}

	fread( mdl->texcoords, sizeof ( struct mdl_texcoord_t ),
		mdl->header.num_verts, fp );
	fread( mdl->triangles, sizeof ( struct mdl_triangle_t ),
		mdl->header.num_tris, fp );

	/* Read frames */
	for( i = 0; i < mdl->header.num_frames; ++i )
	{
		/* Memory allocation for vertices of this frame */
		mdl->frames[i].frame.verts = ( struct mdl_vertex_t * )
			malloc( sizeof ( struct mdl_vertex_t ) * mdl->header.num_verts );

		/* Read frame data */
		fread( &mdl->frames[i].type, sizeof ( long ), 1, fp );
		fread( &mdl->frames[i].frame.bboxmin,
			sizeof ( struct mdl_vertex_t ), 1, fp );
		fread( &mdl->frames[i].frame.bboxmax,
			sizeof ( struct mdl_vertex_t ), 1, fp );
		fread( mdl->frames[i].frame.name, sizeof ( char ), 16, fp );
		fread( mdl->frames[i].frame.verts, sizeof ( struct mdl_vertex_t ),
			mdl->header.num_verts, fp );
	}

	fclose( fp );
	return MDL_to_Mesh( mdl, a_Offset, a_Scale );
}




/**
* Render the model at frame n.
*/
//void RenderFrame (int n, const struct mdl_model_t *mdl)
//{
//  int i, j;
//  GLfloat s, t;
//  vec3_t v;
//  struct mdl_vertex_t *pvert;
//
//  /* Check if n is in a valid range */
//  if ((n < 0) || (n > mdl->header.num_frames - 1))
//    return;
//
//  /* Enable model's texture */
//  glBindTexture (GL_TEXTURE_2D, mdl->tex_id[mdl->iskin]);
//
//  /* Draw the model */
//  glBegin (GL_TRIANGLES);
//    /* Draw each triangle */
//    for (i = 0; i < mdl->header.num_tris; ++i)
//      {
//	/* Draw each vertex */
//	for (j = 0; j < 3; ++j)
//	  {
//	    pvert = &mdl->frames[n].frame.verts[mdl->triangles[i].vertex[j]];
//
//	    /* Compute texture coordinates */
//	    s = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].s;
//	    t = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].t;
//
//	    if (!mdl->triangles[i].facesfront &&
//		mdl->texcoords[mdl->triangles[i].vertex[j]].onseam)
//	      {
//		s += mdl->header.skinwidth * 0.5f; /* Backface */
//	      }
//
//	    /* Scale s and t to range from 0.0 to 1.0 */
//	    s = (s + 0.5) / mdl->header.skinwidth;
//	    t = (t + 0.5) / mdl->header.skinheight;
//
//	    /* Pass texture coordinates to OpenGL */
//	    glTexCoord2f (s, t);
//
//	    /* Normal vector */
//	    glNormal3fv (anorms_table[pvert->normalIndex]);
//
//	    /* Calculate real vertex position */
//	    v[0] = (mdl->header.scale[0] * pvert->v[0]) + mdl->header.translate[0];
//	    v[1] = (mdl->header.scale[1] * pvert->v[1]) + mdl->header.translate[1];
//	    v[2] = (mdl->header.scale[2] * pvert->v[2]) + mdl->header.translate[2];
//
//	    glVertex3fv (v);
//	  }
//      }
//  glEnd ();
//}

/**
* Render the model with interpolation between frame n and n+1.
* interp is the interpolation percent. (from 0.0 to 1.0)
*/
//void RenderFrameItp (int n, float interp, const struct mdl_model_t *mdl)
//{
//  int i, j;
//  GLfloat s, t;
//  vec3_t norm, v;
//  GLfloat *n_curr, *n_next;
//  struct mdl_vertex_t *pvert1, *pvert2;
//
//  /* Check if n is in a valid range */
//  if ((n < 0) || (n > mdl->header.num_frames))
//    return;
//
//  /* Enable model's texture */
//  glBindTexture (GL_TEXTURE_2D, mdl->tex_id[mdl->iskin]);
//
//  /* Draw the model */
//  glBegin (GL_TRIANGLES);
//    /* Draw each triangle */
//    for (i = 0; i < mdl->header.num_tris; ++i)
//      {
//	/* Draw each vertex */
//	for (j = 0; j < 3; ++j)
//	  {
//	    pvert1 = &mdl->frames[n].frame.verts[mdl->triangles[i].vertex[j]];
//	    pvert2 = &mdl->frames[n + 1].frame.verts[mdl->triangles[i].vertex[j]];
//
//	    /* Compute texture coordinates */
//	    s = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].s;
//	    t = (GLfloat)mdl->texcoords[mdl->triangles[i].vertex[j]].t;
//
//	    if (!mdl->triangles[i].facesfront &&
//		mdl->texcoords[mdl->triangles[i].vertex[j]].onseam)
//	      {
//		s += mdl->header.skinwidth * 0.5f; /* Backface */
//	      }
//
//	    /* Scale s and t to range from 0.0 to 1.0 */
//	    s = (s + 0.5) / mdl->header.skinwidth;
//	    t = (t + 0.5) / mdl->header.skinheight;
//
//	    /* Pass texture coordinates to OpenGL */
//	    glTexCoord2f (s, t);
//
//	    /* Interpolate normals */
//	    n_curr = anorms_table[pvert1->normalIndex];
//	    n_next = anorms_table[pvert2->normalIndex];
//
//	    norm[0] = n_curr[0] + interp * (n_next[0] - n_curr[0]);
//	    norm[1] = n_curr[1] + interp * (n_next[1] - n_curr[1]);
//	    norm[2] = n_curr[2] + interp * (n_next[2] - n_curr[2]);
//
//	    glNormal3fv (norm);
//
//	    /* Interpolate vertices */
//	    v[0] = mdl->header.scale[0] * (pvert1->v[0] + interp
//		* (pvert2->v[0] - pvert1->v[0])) + mdl->header.translate[0];
//	    v[1] = mdl->header.scale[1] * (pvert1->v[1] + interp
//		* (pvert2->v[1] - pvert1->v[1])) + mdl->header.translate[1];
//	    v[2] = mdl->header.scale[2] * (pvert1->v[2] + interp
//		* (pvert2->v[2] - pvert1->v[2])) + mdl->header.translate[2];
//
//	    glVertex3fv (v);
//	  }
//      }
//  glEnd ();
//}

/**
* Calculate the current frame in animation beginning at frame
* 'start' and ending at frame 'end', given interpolation percent.
* interp will be reseted to 0.0 if the next frame is reached.
*/
//void Animate (int start, int end, int *frame, float *interp)
//{
//  if ((*frame < start) || (*frame > end))
//    *frame = start;
//
//  if (*interp >= 1.0f)
//    {
//      /* Move to next frame */
//      *interp = 0.0f;
//      (*frame)++;
//
//      if (*frame >= end)
//	*frame = start;
//    }
//}


void cleanup()
{
	FreeModel( &mdlfile );
}

