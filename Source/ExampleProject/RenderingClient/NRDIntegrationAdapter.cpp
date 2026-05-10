#include "NRDIntegrationAdapter.h"

#if INNO_BUILD_WITH_NRD

#include "NRDIntegrationAdapter_Impl.h"

#include "../../Engine/Common/LogService.h"
#include "../../Engine/Engine.h"

using namespace Inno;

NRDIntegrationAdapter::NRDIntegrationAdapter()
{
	m_Impl = new NRDIntegrationAdapterImpl();
}

NRDIntegrationAdapter::~NRDIntegrationAdapter()
{
	if (m_Impl)
	{
		Terminate();
		delete m_Impl;
		m_Impl = nullptr;
	}
}

bool NRDIntegrationAdapter::IsInitialized() const
{
	return m_Impl && m_Impl->m_Initialized;
}

TextureComponent* NRDIntegrationAdapter::GetOutDiffRadianceHitDist() const
{
	return (m_Impl && m_Impl->m_Initialized) ? m_Impl->m_OutDiffShell : nullptr;
}

TextureComponent* NRDIntegrationAdapter::GetOutSpecRadianceHitDist() const
{
	return (m_Impl && m_Impl->m_Initialized) ? m_Impl->m_OutSpecShell : nullptr;
}

#endif // INNO_BUILD_WITH_NRD
