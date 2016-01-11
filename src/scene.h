
#pragma once
#include <string>
#include <vector>
#include "template.h"

#define MAXMATERIAL 256
#define MAXTEXTURES 256

namespace Tmpl8 
{
	class Surface;
	class Mesh;
	class Texture
	{
	public:
	   Texture();
	   ~Texture();

	   void SetSurface( Surface* a_Surface ) { m_surface = a_Surface; }
	   void SetIndex( unsigned int a_Idx ) { m_idx = a_Idx; }
	   void SetName( std::string a_Name );
	   void SetUVScale( float us, float vs ) { m_uScale = us, m_vScale = vs; }

	   std::string GetName() { return m_name; }
	   unsigned int GetIndex() { return m_idx; }
	   float2 GetUVScale() { return float2( m_uScale, m_vScale ); }
	   Surface* GetSurface() { return m_surface; }
	private:
	   Surface* m_surface;
	   std::string m_name;
	   unsigned int m_idx;
	   float m_uScale;
	   float m_vScale;
	};  

	class Material
	{
	public:
		Material();
		~Material();

		void SetName( std::string a_Name );
		void SetDiffuse( unsigned int a_Color ) { m_color = a_Color; }
		void SetIndex( unsigned int a_Idx ) { m_idx = a_Idx; }
		void SetTexture( unsigned int a_Texture ) { m_texture = a_Texture; }
		std::string GetName() { return m_name; }
		unsigned int GetIndex() { return m_idx; }
		unsigned int GetTexture() { return m_texture; }
		unsigned int GetDiffuse() { return m_color; }
	private:
		std::string m_name;
		unsigned int m_color;
		unsigned int m_idx;
		unsigned int m_texture;
	};

	class TextureMgr
	{
	public:
		TextureMgr();
		~TextureMgr();
		Texture* GetTexture( std::string name );
		Texture* GetTexture( int idx );
		Texture* Exists( std::string a_Name );
		unsigned int LoadFile( std::string a_File );
		unsigned int AddTexture( Surface* a_Texture, std::string name );
		static TextureMgr* Get() { return instance; }
		static void Set( TextureMgr* t ) { instance = t; }
	private:
		std::vector<Texture*> m_textures;
		unsigned int m_texCount;
		static TextureMgr* instance;
	};  


	class MaterialMgr
	{
	public:
		MaterialMgr();
		~MaterialMgr();
		void LoadMTL( const char* a_File );
		int AddMat( std::string a_Name, Surface* a_Texture );
		Material* GetMaterial( std::string a_Name );
		Material* GetMaterial( int idx );
		Material* Exists( std::string a_Name );

		static MaterialMgr* Get() { return instance; }
		static void Set( MaterialMgr* m ) { instance = m; }
	private:
		 unsigned int m_matCount;
		 static MaterialMgr* instance;
		std::vector<Material> m_mat;
	}; 

	class Scene
	{
	public:
		Scene();
		~Scene();
		void LoadCollisionMesh( Mesh* mesh );
		std::vector<Mesh*> m_meshlist;
	private:
		MaterialMgr m_materialMgr;
		TextureMgr m_textureMgr;

		void* m_collisionActor;

	};


};