#include "../../Engine/Common/InnoType.h"
#include "../../Engine/Core/IRenderingClient.h"

namespace InnoBaker
{
	void Initialize();
	void LoadScene(std::string& sceneName);
	void BakeSurfelCache();
	void BakeBrickCache(std::string& surfelCacheFileName);
	void BakeBrick(std::string& brickCacheFileName);
	void BakeBrickFactor(std::string& brickFileName);
}

class InnoBakerRenderingClient : public IRenderingClient
{
	// Inherited via IRenderingClient
	virtual bool Setup() override;
	virtual bool Initialize() override;
	virtual bool Render() override;
	virtual bool Terminate() override;
};