#ifndef ECOBASEMANAGER_CPP
#define ECOBASEMANAGER_CPP
#include "EcoBaseManager.h"

using namespace BWAPI::Filter;

EcoBase::EcoBase(BWAPI::Unit center, UnitSeries mineralCluster)
{
    assert(!mineralCluster.empty());  // EcoBase constructor
    Center = center;
    Minerals = mineralCluster;
    assert(false);  // EcoBase constructor
}

void EcoBase::assignMiner(BWAPI::Unit minerUnit)
{
    Workers.insert(minerUnit);
    // ToDo: Command rescuer.
    minerUnit->gather(Minerals[mineralIndex++ % Minerals.size()]);
}

void EcoBase::removeMineral(BWAPI::Unit mineralUnit)
{
    // Consider: Minerals as a reference to benefit map awareness.
    auto endIt = Minerals.end(),
         foundIt = find(Minerals.begin(), endIt, mineralUnit);
    if (foundIt != endIt) {
        Minerals.erase(foundIt);
    }
}

bool EcoBase::isNear(BWAPI::Unit Miner)
{
    auto minerOrder = Miner->getOrder();
    return (minerOrder == BWAPI::Orders::MoveToMinerals ||
            minerOrder == BWAPI::Orders::WaitForMinerals);
}

bool EcoBase::isLackingMiners()
{
    int forgottenMinerals = std::count_if(
            Minerals.begin(), Minerals.end(), &EcoBase::isForgotten);
    int nearlyMining = std::count_if(
            Workers.begin(), Workers.end(), &EcoBase::isNear);
    return forgottenMinerals - nearlyMining > 0;
}

void EcoBaseManager::addBase(BWAPI::Unit baseCenter, UnitSeries mineralCluster)
{
    assert(false);  // addBase
    ++baseAmount;
    mineralAmount += mineralCluster.size();
    EcoBase *Base = new EcoBase(baseCenter, mineralCluster);
    for (BWAPI::Unit Mineral: mineralCluster) {
        unitToBase[Mineral] = Base;
    }
    unitToBase[baseCenter] = Base;
    Bases.push_back(Base);
    assert(false);  // addBase
}

void EcoBaseManager::removeBase(BWAPI::Unit baseCenter)
{
    EcoBase *trash = unitToBase[baseCenter];
    if (!trash) return;  // baseCenter was constructing.
    if (baseCenter != trash->getCenter()) {
        BWAPI::Broodwar->sendTextEx(true, "Wrong center to base");
    }
    BWAPI::Broodwar << "unitToBase size before: " << unitToBase.size()
                    << std::endl;
    // Bug: Does not remove all workers and minerals.
    for (BWAPI::Unit workerUnit: trash->getWorkers()) {
        unitToBase.erase(workerUnit);
        // ToDo: re-assign Workers.
    }
    for (BWAPI::Unit mineralUnit: trash->getMinerals()) {
        unitToBase.erase(mineralUnit);
    }
    unitToBase.erase(baseCenter);
    BWAPI::Broodwar << "unitToBase size after: " << unitToBase.size()
                    << std::endl;
    auto endIt = Bases.end(),
         foundIt = find(Bases.begin(), endIt, trash);
    if (foundIt != endIt) {
        Bases.erase(foundIt);
    }
    delete trash;
    trash = nullptr;
}

void EcoBaseManager::addWorker(BWAPI::Unit workerUnit)
{
    ++workerAmount;
    BWAPI::Unit fromCenter = workerUnit->getClosestUnit(IsResourceDepot, 64);
    EcoBase *Base = unitToBase[fromCenter];
    if (!Base) throw "addWorker: found a missed connection to base";
    Base->assignMiner(workerUnit);
    unitToBase[workerUnit] = Base;
}

// For when the unit is destroyed.
void EcoBaseManager::removeWorker(BWAPI::Unit workerUnit)
{
    --workerAmount;
    EcoBase *Base = unitToBase[workerUnit];
    if (!Base) throw "removeWorker: found a missed connection to base";
    Base->removeMiner(workerUnit);
    unitToBase.erase(workerUnit);
}

void EcoBaseManager::removeMineral(BWAPI::Unit mineralUnit)
{
    --mineralAmount;
    EcoBase *Base = unitToBase[mineralUnit];
    if (!Base) throw "removeMineral: found a missed connection to base";
    Base->removeMineral(mineralUnit);
    unitToBase.erase(mineralUnit);
}

void EcoBaseManager::produceUnits(BWAPI::UnitType unitType)
{
    BWAPI::Unit centerUnit = nullptr;
    for (EcoBase *Base: Bases) {
        centerUnit = Base->getCenter();
        if (Utils::isIdle(centerUnit) && Base->isLackingMiners()) {
            centerUnit->train(unitType);
        }
    }
}

bool EcoBaseManager::isAtCapacity()
{
    for (EcoBase *Base: Bases) {
        if (Base->isLackingMiners()) {
            return false;
        }
    }
    return true;
}

void EcoBaseManager::displayStatus(int &row)
{
    BWAPI::Broodwar->drawTextScreen(3, row,
        "Total - Bases %d, Minerals: %d, Workers: %d.",
        getBaseAmount(), getMineralAmount(), getWorkerAmount());
    for (EcoBase *Base: Bases) {
        row += 10;
        BWAPI::Broodwar->drawTextScreen(3, row,
            "ID: %d, Minerals: %d, Miners: %d, LackingMiners %s.",
            Base->getCenter()->getID(), Base->getMineralCount(),
            Base->getMinerCount(), Base->isLackingMiners() ? "true" : "false");
    }
    row += 5;
}

#endif
