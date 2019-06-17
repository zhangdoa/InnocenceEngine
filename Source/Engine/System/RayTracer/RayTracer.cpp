#include "RayTracer.h"

#include "../ICoreSystem.h"
extern ICoreSystem* g_pCoreSystem;

namespace RayTracerNS
{
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool InnoRayTracer::Setup()
{
	RayTracerNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoRayTracer::Initialize()
{
	std::ofstream l_ExportFile(g_pCoreSystem->getFileSystem()->getWorkingDirectory() + "//Res//test.ppm", std::ios::out | std::ios::trunc);

	auto l_ScreenResolution = g_pCoreSystem->getRenderingFrontend()->getScreenResolution();

	int nx = l_ScreenResolution.x;
	int ny = l_ScreenResolution.y;

	l_ExportFile << "P3\n" << nx << " " << ny << "\n255\n";

	for (int j = ny - 1; j >= 0; j--) {
		for (int i = 0; i < nx; i++) {
			float r = float(i) / float(nx);
			float g = float(j) / float(ny);
			float b = 0.2f;
			int ir = int(255.99*r);
			int ig = int(255.99*g);
			int ib = int(255.99*b);
			l_ExportFile << ir << " " << ig << " " << ib << "\n";
		}
	}

	l_ExportFile.close();

	RayTracerNS::m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool InnoRayTracer::Update()
{
	return true;
}

bool InnoRayTracer::Terminate()
{
	RayTracerNS::m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus InnoRayTracer::GetStatus()
{
	return ObjectStatus();
}