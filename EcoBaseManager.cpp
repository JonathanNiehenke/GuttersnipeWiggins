#ifndef ECOBASEMANAGER_CPP
#define ECOBASEMANAGER_CPP
#include "EcoBaseManager.h"

using namespace BWAPI::Filter;

EcoBase::EcoBase(BWAPI::Unit center, BWAPI::Unitset mineralCluster)
{
    Center = center;
    Minerals.assign(mineralCluster.begin(), mineralCluster.end());
    std::sort(Minerals.begin(), Minerals.end(),
              Utils::compareDistanceFrom(center));
}

void EcoBase::assignMiner(BWAPI::Unit minerUnit)
{
    Workers.insert(minerUnit);
    minerUnit->gather(Minerals[mineralIndex++ % Minerals.size()]);
}

void EcoBase::releaseMiner(BWAPI::Unit minerUnit)
{
    Workers.erase(minerUnit);
    minerUnit->stop();
}

void EcoBase::removeMineral(BWAPI::Unit mineralUnit)
{
    // Consider: Minerals as a reference benefitting map awareness.
    Minerals.erase(find(Minerals.begin(), Minerals.end(), mineralUnit));
}

bool EcoBase::isForgotten(BWAPI::Unit Mineral) {
    // Believing it's false even if miner is in motion or waiting.
    return !Mineral->isBeingGathered();  
}

bool EcoBase::isNear(BWAPI::Unit Miner) {
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


EcoBaseManager::~EcoBaseManager()
{
    for (EcoBase* Base: Bases) {
        delete Base;
    }
}

void EcoBaseManager::addBase(
        BWAPI::Unit baseCenter, BWAPI::Unitset  mineralCluster)
{
    ++baseAmount;
    mineralAmount += mineralCluster.size();
    EcoBase *Base = new EcoBase(baseCenter, mineralCluster);
    for (BWAPI::Unit Mineral: mineralCluster) {
        unitToBase[Mineral] = Base;
    }
    unitToBase[baseCenter] = Base;
    Bases.insert(Base);
}

void EcoBaseManager::removeBase(BWAPI::Unit baseCenter)
{
    EcoBase *trash = unitToBase[baseCenter];
    for (BWAPI::Unit workerUnit: trash->getWorkers()) {
        unitToBase.erase(workerUnit);
        // ToDo: re-assign Workers.
    }
    for (BWAPI::Unit mineralUnit: trash->getMinerals()) {
        unitToBase.erase(mineralUnit);
    }
    unitToBase.erase(baseCenter);
    Bases.erase(trash);
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
    Base->releaseMiner(workerUnit);
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

// Usually for overlords.
void EcoBaseManager::produceSingleUnit(BWAPI::UnitType unitType)
{
    BWAPI::Unit centerUnit = nullptr;
    for (EcoBase *Base: Bases) {
        if (!Base) throw "produceS: found a missed connection to base";
        centerUnit = Base->getCenter();
        if (Utils::isIdle(centerUnit)) {
            centerUnit->train(unitType);
            break;
        }
    }
}

void EcoBaseManager::produceUnits(BWAPI::UnitType unitType)
{
    BWAPI::Unit centerUnit = nullptr;
    for (EcoBase *Base: Bases) {
        if (!Base) throw "produce: found a missed connection to base";
        centerUnit = Base->getCenter();
        if (Utils::isIdle(centerUnit) && Base->isLackingMiners()) {
            centerUnit->train(unitType);
        }
    }
}

bool EcoBaseManager::isAtCapacity()
{
    for (EcoBase *Base: Bases) {
        if (!Base) throw "capacity: found a missed connection to base";
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
        if (!Base) throw "Bad to base.";
        row += 10;
        BWAPI::Broodwar->drawTextScreen(3, row,
            "ID: %d, Minerals: %d, Miners: %d, LackingMiners %s.",
            Base->getCenter()->getID(), Base->getMineralCount(),
            Base->getMinerCount(), Base->isLackingMiners() ? "true" : "false");
    }
    row += 5;
}

#endif
