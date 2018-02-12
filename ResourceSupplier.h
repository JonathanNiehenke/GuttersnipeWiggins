#pragma once
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
        UnitSeries Refineries;
        int mineralIndex = 0;  // For miner to mineral assignment.
        bool noWorkersApproachingGas() const;
        void assignGasGathererTo(const BWAPI::Unit& refinery) const;
    public:
        EcoBase(BWAPI::Unit center, UnitSeries mineralCluster);
        BWAPI::Unit getCenter() { return Center; }
        UnitGroup getWorkers() { return Workers; }
        UnitSeries getMinerals() { return Minerals; }
        int getMineralCount() { return Minerals.size(); }
        int getMinerCount() { return Workers.size(); }
        void assignMiner(BWAPI::Unit minerUnit);
        void includeRefinery(const BWAPI::Unit& refineryUnit);
        void removeMiner(BWAPI::Unit minerUnit) { Workers.erase(minerUnit); }
        void removeMineral(BWAPI::Unit Mineral);
        static bool isForgotten(BWAPI::Unit Mineral)
            { return !Mineral->isBeingGathered(); }
        static bool isNear(BWAPI::Unit Miner);
        bool isLackingMiners();
        void fillLackingGas() const;
};


// Manages all bases of workers and minerals.
class ResourceSupplier
{
    typedef std::unordered_set<BWAPI::Unit> UnitGroup;
    typedef std::vector<BWAPI::Unit> UnitSeries;
    private:
        int baseAmount = 0, mineralAmount = 0, workerAmount = 0,
            pendingWorkers = 0;
        BWAPI::UnitType workerType;
        // For unit lookup of assigned Base.
        std::unordered_map<BWAPI::Unit, EcoBase*> unitToBase;
        std::vector<EcoBase*> Bases;
        void initialWorkaround(const BWAPI::Unit& workerUnit);
        static bool isQueueEmpty(const BWAPI::Unit& Base);
    public:
        ResourceSupplier(const BWAPI::UnitType& workerType);
        ~ResourceSupplier();
        int getBaseAmount() { return baseAmount; }
        int getMineralAmount() { return mineralAmount; }
        int getWorkerAmount() { return workerAmount; }
        void addBase(const BWAPI::Unit& baseCenter);
        std::vector<BWAPI::Unit> getNearbyMinerals(const BWAPI::Unit&);
        void removeBase(BWAPI::Unit baseCenter);
        void addWorker(const BWAPI::Unit& workerUnit);
        void removeWorker(BWAPI::Unit workerUnit);
        void addRefinery(const BWAPI::Unit& refineryUnit);
        void removeMineral(BWAPI::Unit mineralUnit);
        bool canFillLackingMiners();
        bool isAtCapacity();
        void fillLackingGasGathers() const;
        BWAPI::Unit getFirstCenter() { return Bases.front()->getCenter(); }
        BWAPI::Unit getLastCenter() { return Bases.back()->getCenter(); }
        void displayStatus(int &row);
};
