#pragma once
#include "IEventManager.h"
#include "Math.h"
#include "UIManager.h"
#include "GLRenderingManager.h"

namespace nmsp_GraphicManager
{
	class Vertex
	{
	public:
		Vertex();
		~Vertex();

		const Vec3f& getPos();
		const Vec2f& getTexCoord();
		const Vec3f& getNormal();

		void setPos(const Vec3f& pos);
		void setTexCoord(const Vec2f& texCoord);
		void setNormal(const Vec3f& normal);

		void addVertexData(const Vec3f & pos, const Vec2f & texCoord, const Vec3f & normal);
	private:
		Vec3f m_pos;
		Vec2f m_texCoord;
		Vec3f m_normal;
	};

	class Mesh
	{
	public:
		Mesh();
		~Mesh();

		void init();
		void update();
		void shutdown();

		void addMeshData(std::vector<Vertex*>& vertices, std::vector<unsigned int>& indices, bool calcNormals);
		void addTestTriangle();

	private:
		GLuint m_vertexArrayID;
		GLuint m_VBO;
		GLuint m_VAO;
		GLuint m_IBO;

		std::vector<Vertex*> m_vertices;
		std::vector<unsigned int> m_intices;

	};


	class GraphicManager : public IEventManager
	{
	public:
		GraphicManager();
		~GraphicManager();

	private:
		void init() override;
		void update() override;
		void shutdown() override;

		UIManager m_uiManager;
		GLRenderingManager m_renderingManager;

		Mesh testTriangle;

	};
}

