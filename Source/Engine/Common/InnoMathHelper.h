#pragma once
#include "InnoMath.h"

namespace InnoMath
{
	template<class T>
	inline auto generateNDC(TVertex<T>* vertices)
	{
		vertices[0].m_pos = TVec4<T>(one<T>, one<T>, one<T>, one<T>);
		vertices[0].m_texCoord = TVec2<T>(one<T>, one<T>);

		vertices[1].m_pos = TVec4<T>(one<T>, -one<T>, one<T>, one<T>);
		vertices[1].m_texCoord = TVec2<T>(one<T>, zero<T>);

		vertices[2].m_pos = TVec4<T>(-one<T>, -one<T>, one<T>, one<T>);
		vertices[2].m_texCoord = TVec2<T>(zero<T>, zero<T>);

		vertices[3].m_pos = TVec4<T>(-one<T>, one<T>, one<T>, one<T>);
		vertices[3].m_texCoord = TVec2<T>(zero<T>, one<T>);

		vertices[4].m_pos = TVec4<T>(one<T>, one<T>, -one<T>, one<T>);
		vertices[4].m_texCoord = TVec2<T>(one<T>, one<T>);

		vertices[5].m_pos = TVec4<T>(one<T>, -one<T>, -one<T>, one<T>);
		vertices[5].m_texCoord = TVec2<T>(one<T>, zero<T>);

		vertices[6].m_pos = TVec4<T>(-one<T>, -one<T>, -one<T>, one<T>);
		vertices[6].m_texCoord = TVec2<T>(zero<T>, zero<T>);

		vertices[7].m_pos = TVec4<T>(-one<T>, one<T>, -one<T>, one<T>);
		vertices[7].m_texCoord = TVec2<T>(zero<T>, one<T>);

		for (size_t i = 0; i < 8; i++)
		{
			vertices[i].m_normal = TVec4<T>(vertices[i].m_pos.x, vertices[i].m_pos.y, vertices[i].m_pos.z, zero<T>).normalize();
		}
	}

	template<class T>
	inline auto generateAABB(TVec4<T> boundMax, TVec4<T> boundMin) -> TAABB<T>
	{
		TAABB<T> l_result;

		l_result.m_boundMin = boundMin;
		l_result.m_boundMax = boundMax;

		l_result.m_center = (boundMax + boundMin) * half<T>;
		l_result.m_extend = boundMax - boundMin;
		l_result.m_extend.w = one<T>;

		return l_result;
	}

	template<class T>
	inline auto generateAABB(TVertex<T>* vertices, size_t size) -> TAABB<T>
	{
		TAABB<T> l_result;

		T maxX = vertices[0].m_pos.x;
		T maxY = vertices[0].m_pos.y;
		T maxZ = vertices[0].m_pos.z;
		T minX = vertices[0].m_pos.x;
		T minY = vertices[0].m_pos.y;
		T minZ = vertices[0].m_pos.z;

		for (size_t i = 0; i < size; i++)
		{
			if (vertices[i].m_pos.x >= maxX)
			{
				maxX = vertices[i].m_pos.x;
			}
			if (vertices[i].m_pos.y >= maxY)
			{
				maxY = vertices[i].m_pos.y;
			}
			if (vertices[i].m_pos.z >= maxZ)
			{
				maxZ = vertices[i].m_pos.z;
			}
			if (vertices[i].m_pos.x <= minX)
			{
				minX = vertices[i].m_pos.x;
			}
			if (vertices[i].m_pos.y <= minY)
			{
				minY = vertices[i].m_pos.y;
			}
			if (vertices[i].m_pos.z <= minZ)
			{
				minZ = vertices[i].m_pos.z;
			}
		}

		return generateAABB(TVec4(maxX, maxY, maxZ, one<T>), TVec4(minX, minY, minZ, one<T>));

		return l_result;
	}

	template<class T>
	inline auto generateBoundSphere(const TAABB<T>& rhs) -> TSphere<T>
	{
		TSphere<T> l_result;
		l_result.m_center = rhs.m_center;
		l_result.m_radius = (rhs.m_boundMax - rhs.m_center).length();

		return l_result;
	}

	template<class T>
	inline auto makeFrustum(const TVertex<T>* vertices) -> TFrustum<T>
	{
		TFrustum<T> l_result;

		l_result.m_px = makePlane(vertices[2].m_pos, vertices[6].m_pos, vertices[7].m_pos);
		l_result.m_nx = makePlane(vertices[0].m_pos, vertices[4].m_pos, vertices[1].m_pos);
		l_result.m_py = makePlane(vertices[0].m_pos, vertices[3].m_pos, vertices[7].m_pos);
		l_result.m_ny = makePlane(vertices[1].m_pos, vertices[5].m_pos, vertices[6].m_pos);
		l_result.m_pz = makePlane(vertices[0].m_pos, vertices[1].m_pos, vertices[2].m_pos);
		l_result.m_nz = makePlane(vertices[4].m_pos, vertices[7].m_pos, vertices[6].m_pos);

		return l_result;
	}

	template<class T>
	inline auto transformAABBSpace(const TAABB<T>& rhs, TMat4<T> Tm) -> TAABB<T>
	{
		TAABB<T> l_result;

		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_result.m_boundMax = InnoMath::mul(rhs.m_boundMax, Tm);
		l_result.m_boundMin = InnoMath::mul(rhs.m_boundMin, Tm);
		l_result.m_center = InnoMath::mul(rhs.m_center, Tm);
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		l_result.m_boundMax = InnoMath::mul(Tm, rhs.m_boundMax);
		l_result.m_boundMin = InnoMath::mul(Tm, rhs.m_boundMin);
		l_result.m_center = InnoMath::mul(Tm, rhs.m_center);
#endif

		l_result.m_extend = l_result.m_boundMax - l_result.m_boundMin;
		l_result.m_extend.w = one<T>;

		return l_result;
	};

	inline std::vector<Vertex> worldToViewSpace(const std::vector<Vertex>& rhs, mat4 t, mat4 r)
	{
		auto l_result = rhs;

		for (auto& l_vertexData : l_result)
		{
			auto l_mulPos = InnoMath::worldToViewSpace(l_vertexData.m_pos, t, r);
			l_vertexData.m_pos = l_mulPos;
		}

		for (auto& l_vertexData : l_result)
		{
			l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
		}

		return l_result;
	}

	inline std::vector<Vertex> viewToWorldSpace(const std::vector<Vertex>& rhs, mat4 t, mat4 r)
	{
		auto l_result = rhs;

		for (auto& l_vertexData : l_result)
		{
			auto l_mulPos = InnoMath::viewToWorldSpace(l_vertexData.m_pos, t, r);
			l_vertexData.m_pos = l_mulPos;
		}

		for (auto& l_vertexData : l_result)
		{
			l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
		}

		return l_result;
	}

	inline std::vector<Vertex> generateFrustumVerticesVS(mat4 p)
	{
		std::vector<Vertex> rhs(8);

		InnoMath::generateNDC<float>(&rhs[0]);

		for (auto& i : rhs)
		{
			i.m_pos = InnoMath::clipToViewSpace(i.m_pos, p);
		}

		// near clip plane first
		// @TODO: reverse only along Z axis, not simple mirrored version
		std::reverse(rhs.begin(), rhs.end());

		std::vector<Vertex> l_vertices(8);

		for (unsigned int i = 0; i < 8; i++)
		{
			l_vertices[i] = rhs[i];
		}

		return l_vertices;
	}

	inline std::vector<Vertex> generateFrustumVerticesWS(mat4 p, mat4 r, mat4 t)
	{
		auto rhs = generateFrustumVerticesVS(p);

		for (auto& i : rhs)
		{
			i.m_pos = InnoMath::viewToWorldSpace(i.m_pos, t, r);
		}

		for (auto& i : rhs)
		{
			i.m_normal = vec4(i.m_pos.x, i.m_pos.y, i.m_pos.z, 0.0f).normalize();
		}

		return rhs;
	}

	inline std::vector<Vertex> generateAABBVertices(vec4 boundMax, vec4 boundMin)
	{
		std::vector<Vertex> l_vertices(8);

		l_vertices[0].m_pos = (vec4(boundMax.x, boundMax.y, boundMax.z, 1.0f));
		l_vertices[0].m_texCoord = vec2(1.0f, 1.0f);

		l_vertices[1].m_pos = (vec4(boundMax.x, boundMin.y, boundMax.z, 1.0f));
		l_vertices[1].m_texCoord = vec2(1.0f, 0.0f);

		l_vertices[2].m_pos = (vec4(boundMin.x, boundMin.y, boundMax.z, 1.0f));
		l_vertices[2].m_texCoord = vec2(0.0f, 0.0f);

		l_vertices[3].m_pos = (vec4(boundMin.x, boundMax.y, boundMax.z, 1.0f));
		l_vertices[3].m_texCoord = vec2(0.0f, 1.0f);

		l_vertices[4].m_pos = (vec4(boundMax.x, boundMax.y, boundMin.z, 1.0f));
		l_vertices[4].m_texCoord = vec2(1.0f, 1.0f);

		l_vertices[5].m_pos = (vec4(boundMax.x, boundMin.y, boundMin.z, 1.0f));
		l_vertices[5].m_texCoord = vec2(1.0f, 0.0f);

		l_vertices[6].m_pos = (vec4(boundMin.x, boundMin.y, boundMin.z, 1.0f));
		l_vertices[6].m_texCoord = vec2(0.0f, 0.0f);

		l_vertices[7].m_pos = (vec4(boundMin.x, boundMax.y, boundMin.z, 1.0f));
		l_vertices[7].m_texCoord = vec2(0.0f, 1.0f);

		for (auto& l_vertexData : l_vertices)
		{
			l_vertexData.m_normal = vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
		}

		return l_vertices;
	}

	inline std::vector<Vertex> generateAABBVertices(const AABB& rhs)
	{
		auto boundMax = rhs.m_boundMax;
		auto boundMin = rhs.m_boundMin;

		return generateAABBVertices(boundMax, boundMin);
	}
}