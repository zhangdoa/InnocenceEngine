#include "Serializer.h"

#include "../../Engine/Common/MathHelper.h"

#include "../../Engine/Engine.h"

using namespace Inno;


#include "../../Engine/Common/IOService.h"
#include "Baker.h"



namespace Inno
{
    bool Baker::serializeProbeInfos(const ProbeInfo& probeInfo)
    {
        auto l_filePath = g_Engine->Get<IOService>()->getWorkingDirectory();

        std::ofstream l_file;
        l_file.open(l_filePath + "..//Res//Scenes//" + Config::Get().m_exportFileName + ".InnoProbeInfo", std::ios::out | std::ios::trunc | std::ios::binary);
        l_file.write((char*)&probeInfo, sizeof(probeInfo));
        l_file.close();

        return true;
    }

    bool Baker::serializeProbes(const std::vector<Probe>& probes)
    {
        auto l_filePath = g_Engine->Get<IOService>()->getWorkingDirectory();

        std::ofstream l_file;
        l_file.open(l_filePath + "..//Res//Scenes//" + Config::Get().m_exportFileName + ".InnoProbe", std::ios::out | std::ios::trunc | std::ios::binary);
        IOService::serializeVector(l_file, probes);
        l_file.close();

        Log(Success, "", l_filePath.c_str(), "..//Res//Scenes//", Config::Get().m_exportFileName.c_str(), ".Probe has been saved.");

        return true;
    }

    bool Baker::serializeSurfels(const std::vector<Surfel>& surfels)
    {
        auto l_filePath = g_Engine->Get<IOService>()->getWorkingDirectory();

        std::ofstream l_file;
        l_file.open(l_filePath + "..//Res//Scenes//" + Config::Get().m_exportFileName + ".InnoSurfel", std::ios::out | std::ios::trunc | std::ios::binary);
        IOService::serializeVector(l_file, surfels);
        l_file.close();

        Log(Success, "", l_filePath.c_str(), "..//Res//Scenes//", Config::Get().m_exportFileName.c_str(), ".InnoSurfel has been saved.");

        return true;
    }

    bool Baker::serializeSurfelCaches(const std::vector<Surfel>& surfelCaches)
    {
        auto l_filePath = g_Engine->Get<IOService>()->getWorkingDirectory();

        std::ofstream l_file;
        l_file.open(l_filePath + "..//Res//Intermediate//" + Config::Get().m_exportFileName + ".InnoSurfelCache", std::ios::out | std::ios::trunc | std::ios::binary);
        IOService::serializeVector(l_file, surfelCaches);
        l_file.close();

        Log(Success, "", l_filePath.c_str(), "..//Res//Intermediate//", Config::Get().m_exportFileName.c_str(), ".InnoSurfelCache has been saved.");

        return true;
    }

    bool Baker::serializeBrickCaches(const std::vector<BrickCache>& brickCaches)
    {
        auto l_brickCacheCount = brickCaches.size();

        // Serialize metadata
        auto l_filePath = g_Engine->Get<IOService>()->getWorkingDirectory();
        std::ofstream l_summaryFile;
        l_summaryFile.open(l_filePath + "..//Res//Intermediate//" + Config::Get().m_exportFileName + ".InnoBrickCacheSummary", std::ios::out | std::ios::trunc | std::ios::binary);

        std::vector<BrickCacheSummary> l_brickCacheSummaries;
        l_brickCacheSummaries.reserve(l_brickCacheCount);

        for (size_t i = 0; i < l_brickCacheCount; i++)
        {
            BrickCacheSummary l_brickCacheSummary;

            l_brickCacheSummary.pos = brickCaches[i].pos;
            l_brickCacheSummary.fileIndex = i;
            l_brickCacheSummary.fileSize = brickCaches[i].surfelCaches.size();

            l_brickCacheSummaries.emplace_back(l_brickCacheSummary);
        }

        IOService::serializeVector(l_summaryFile, l_brickCacheSummaries);

        l_summaryFile.close();

        // Serialize surfels cache for each brick
        std::ofstream l_surfelCacheFile;
        l_surfelCacheFile.open(l_filePath + "..//Res//Intermediate//" + Config::Get().m_exportFileName + ".InnoBrickCache", std::ios::out | std::ios::trunc | std::ios::binary);

        for (size_t i = 0; i < l_brickCacheCount; i++)
        {
            IOService::serializeVector(l_surfelCacheFile, brickCaches[i].surfelCaches);
        }
        l_surfelCacheFile.close();

        Log(Success, "", l_filePath.c_str(), "..//Res//Intermediate//", Config::Get().m_exportFileName.c_str(), ".InnoBrickCacheSummary has been saved.");

        return true;
    }

    bool Baker::deserializeBrickCaches(const std::vector<BrickCacheSummary>& brickCacheSummaries, std::vector<BrickCache>& brickCaches)
    {
        auto l_brickCount = brickCacheSummaries.size();
        brickCaches.reserve(l_brickCount);

        auto l_filePath = g_Engine->Get<IOService>()->getWorkingDirectory();

        std::ifstream l_file;
        l_file.open(l_filePath + "..//Res//Intermediate//" + Config::Get().m_exportFileName + ".InnoBrickCache", std::ios::binary);

        size_t l_startOffset = 0;

        for (size_t i = 0; i < l_brickCount; i++)
        {
            BrickCache l_brickCache;
            l_brickCache.pos = brickCacheSummaries[i].pos;

            auto l_fileSize = brickCacheSummaries[i].fileSize;
            l_brickCache.surfelCaches.resize(l_fileSize);

            IOService::deserializeVector(l_file, l_startOffset * sizeof(Surfel), l_fileSize * sizeof(Surfel), l_brickCache.surfelCaches);

            l_startOffset += l_fileSize;

            brickCaches.emplace_back(std::move(l_brickCache));
        }

        return true;
    }

    bool Baker::serializeBricks(const std::vector<Brick>& bricks)
    {
        auto l_filePath = g_Engine->Get<IOService>()->getWorkingDirectory();

        std::ofstream l_file;
        l_file.open(l_filePath + "..//Res//Scenes//" + Config::Get().m_exportFileName + ".InnoBrick", std::ios::out | std::ios::trunc | std::ios::binary);
        IOService::serializeVector(l_file, bricks);
        l_file.close();

        Log(Success, "", l_filePath.c_str(), "..//Res//Scenes//", Config::Get().m_exportFileName.c_str(), ".InnoBrick has been saved.");

        return true;
    }

    bool Baker::serializeBrickFactors(const std::vector<BrickFactor>& brickFactors)
    {
        auto l_filePath = g_Engine->Get<IOService>()->getWorkingDirectory();

        std::ofstream l_file;
        l_file.open(l_filePath + "..//Res//Scenes//" + Config::Get().m_exportFileName + ".InnoBrickFactor", std::ios::out | std::ios::trunc | std::ios::binary);
        IOService::serializeVector(l_file, brickFactors);
        l_file.close();

        Log(Success, "", l_filePath.c_str(), "..//Res//Scenes//", Config::Get().m_exportFileName.c_str(), ".InnoBrickFactor has been saved.");

        return true;
    }
}
