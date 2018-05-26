#pragma once
#include "interface/IGraphicPrimitive.h"
#include "InnoMath.h"
#include "interface/IMemorySystem.h"
#include "interface/ILogSystem.h"
extern IMemorySystem* g_pMemorySystem;
extern ILogSystem* g_pLogSystem;

class BaseMesh : public IGraphicPrimitive
{
public:
	BaseMesh() { m_meshID = std::rand(); };
	virtual ~BaseMesh() {};

	void setup() override;
	void setup(meshType meshType, meshDrawMethod meshDrawMethod, bool calculateNormals, bool calculateTangents);
	void initialize() override;
	virtual void draw() = 0;
	const objectStatus& getStatus() const override;
	const meshID getMeshID() const;
	const vec4 findMaxVertex() const;
	const vec4 findMinVertex() const;
	void addVertices(const Vertex& Vertex);
	void addVertices(const vec4 & pos, const vec2 & texCoord, const vec4 & normal);
	void addVertices(double pos_x, double pos_y, double pos_z, double texCoord_x, double texCoord_y, double normal_x, double normal_y, double normal_z);
	void addIndices(unsigned int index);

	void addUnitCube();
	void addUnitSphere();
	void addUnitQuad();
	void addUnitLine();

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	meshID m_meshID;
	meshType m_meshType;
	std::vector<Vertex> m_vertices;
	std::vector<unsigned int> m_indices;

	meshDrawMethod m_meshDrawMethod;
	bool m_calculateNormals;
	bool m_calculateTangents;
};
