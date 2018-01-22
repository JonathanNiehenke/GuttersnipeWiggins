#ifndef ECOBASEMANAGER_H
#define ECOBASEMANAGER_H
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>
#include <cassert>
#include <BWAPI.h>
#include "Utils.h"

class EcoBase
{
    typedef std::unordered_set<BWAPI::Unit> UnitGroup;
    typedef std::vector<BWAPI::Unit> UnitSeries;
    private:
        BWAPI::Unit Center = nullptr;
        UnitGroup Workers;
        UnitSeries Minerals;
        int mineralIndex = 0;  // For miner to mineral assignment.
    public:
        EcoBase(BWAPI::Unit center, UnitSeries mineralCluster);
        BWAPI::Unit getCenter() { return Center; }
        UnitGroup getWorkers() { return Workers; }
        UnitSeries getMinerals() { return Minerals; }
        int getMineralCount() { return Minerals.size(); }
        int getMinerCount() { return Workers.size(); }
        void assignMiner(BWAPI::Unit minerUnit);
        void removeMiner(BWAPI::Unit minerUnit) { Workers.erase(minerUnit); }
        void removeMineral(BWAPI::Unit Mineral);
        static bool isForgotten(BWAPI::Unit Mineral)
            { return !Mineral->isBeingGathered(); }
        static bool isNear(BWAPI::Unit Miner);
        bool isLackingMiners();
};


// Manages all bases of workers and minerals.
class EcoBaseManager
{
    typedef std::unordered_set<BWAPI::Unit> UnitGroup;
    typedef std::vector<BWAPI::Unit> UnitSeries;
    private:
        int baseAmount = 0, mineralAmount = 0, workerAmount = 0,
            pendingWorkers = 0;
        // For unit lookup of assigned Base.
        std::unordered_map<BWAPI::Unit, EcoBase*> unitToBase;
        std::vector<EcoBase*> Bases;
    public:
        ~EcoBaseManager() { for (EcoBase* Base: Bases) delete Base; }
        int getBaseAmount() { return baseAmount; }
        int getMineralAmount() { return mineralAmount; }
        int getWorkerAmount() { return workerAmount; }
        void addBase(BWAPI::Unit baseCenter, UnitSeries mineralCluster);
        void debugSize() {BWAPI::Broodwar << "unitToBase size around: "
            << unitToBase.size() << std::endl; }
        void removeBase(BWAPI::Unit baseCenter);
        void addWorker(BWAPI::Unit workerUnit);
        void removeWorker(BWAPI::Unit workerUnit);
        void removeMineral(BWAPI::Unit mineralUnit);
        bool canFillLackingMiners();
        void produceUnits(BWAPI::UnitType unitType);
        bool isAtCapacity();
        BWAPI::Unit getFirstCenter() { return Bases.front()->getCenter(); }
        BWAPI::Unit getLastCenter() { return Bases.back()->getCenter(); }
        void displayStatus(int &row);
};

#endif
