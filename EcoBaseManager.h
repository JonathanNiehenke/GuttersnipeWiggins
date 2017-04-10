#ifndef ECOBASEMANAGER_H
#define ECOBASEMANAGER_H
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <vector>
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
        EcoBase(BWAPI::Unit center, BWAPI::Unitset mineralCluster);
        BWAPI::Unit getCenter() { return Center; }
        UnitGroup getWorkers() { return Workers; }
        UnitSeries getMinerals() { return Minerals; }
        int getMineralCount() { return Minerals.size(); }
        int getMinerCount() { return Workers.size(); }
        void assignMiner(BWAPI::Unit minerUnit);
        void releaseMiner(BWAPI::Unit minerUnit);
        void removeMineral(BWAPI::Unit Mineral);
        static bool isForgotten(BWAPI::Unit Mineral);
        static bool isNear(BWAPI::Unit Miner);
        bool isLackingMiners();
};


// Manages all bases of workers and minerals.
class EcoBaseManager
{
    private:
        typedef std::unordered_set<BWAPI::Unit> UnitGroup;
        int baseAmount = 0, mineralAmount = 0, workerAmount = 0,
            pendingWorkers = 0;
        // Why: unit lookup for assigned Base->
        std::unordered_map<BWAPI::Unit, EcoBase*> unitToBase;
        std::unordered_set<EcoBase*> Bases;
    public:
        ~EcoBaseManager();
        int getBaseAmount() { return baseAmount; }
        int getMineralAmount() { return mineralAmount; }
        int getWorkerAmount() { return workerAmount; }
        void addBase(BWAPI::Unit baseCenter, BWAPI::Unitset  mineralCluster);
        void removeBase(BWAPI::Unit baseCenter);
        void addWorker(BWAPI::Unit workerUnit);
        void removeWorker(BWAPI::Unit workerUnit);
        void removeMineral(BWAPI::Unit mineralUnit);
        void produceUnits(BWAPI::UnitType unitType);
        bool isAtCapacity();
        BWAPI::Unit getCenter() { return (*Bases.begin())->getCenter(); }
        void displayStatus(int &row);
};

#endif
