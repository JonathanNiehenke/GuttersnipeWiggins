#pragma once
#include "ResourceSupplier.h"

using namespace BWAPI::Filter;

EcoBase::EcoBase(
    BWAPI::Unit center, UnitSeries mineralCluster)
{
    Center = center;
    Minerals = mineralCluster;
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

ResourceSupplier::ResourceSupplier(const BWAPI::UnitType& workerType) {
    this->workerType = workerType;
}

ResourceSupplier::~ResourceSupplier() {
    for (EcoBase* Base: Bases) delete Base;
}

void ResourceSupplier::addBase(const BWAPI::Unit& baseCenter) {
    std::vector<BWAPI::Unit> nearbyMinerals = getNearbyMinerals(baseCenter);
    BWAPI::Broodwar << "Found " << nearbyMinerals.size() << " minerals"
                    << std::endl;
    EcoBase* Base = new EcoBase(baseCenter, nearbyMinerals);
    for (BWAPI::Unit Mineral: nearbyMinerals) {
        unitToBase[Mineral] = Base;
    }
    unitToBase[baseCenter] = Base;
    Bases.push_back(Base);
    mineralAmount += nearbyMinerals.size();
    ++baseAmount;
}

 std::vector<BWAPI::Unit> ResourceSupplier::getNearbyMinerals(
    const BWAPI::Unit& baseCenter)
{
    const BWAPI::Unitset nearbyMinerals = baseCenter->getUnitsInRadius(
        200, IsMineralField);
    std::vector<BWAPI::Unit> sortedMinerals = std::vector<BWAPI::Unit>(
        nearbyMinerals.begin(), nearbyMinerals.end());
    std::sort(sortedMinerals.begin(), sortedMinerals.end(),
        Utils::Position::fromUnit(baseCenter).compareUnits());
    return sortedMinerals;
}

void ResourceSupplier::removeBase(BWAPI::Unit baseCenter)
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

void ResourceSupplier::addWorker(const BWAPI::Unit& workerUnit)
{
    BWAPI::Unit fromCenter = workerUnit->getClosestUnit(IsResourceDepot, 64);
    EcoBase* Base = unitToBase[fromCenter];
    if (Base) {
        Base->assignMiner(workerUnit);
        unitToBase[workerUnit] = Base;
        ++workerAmount;
    }
    else if (BWAPI::Broodwar->getFrameCount() < 10)
        initialWorkaround(workerUnit);
}

void ResourceSupplier::initialWorkaround(const BWAPI::Unit& workerUnit) {
    BWAPI::Broodwar->registerEvent(
        [workerUnit, this](BWAPI::Game*) { this->addWorker(workerUnit); },
        nullptr, 1);
}

// For when the unit is destroyed.
void ResourceSupplier::removeWorker(BWAPI::Unit workerUnit)
{
    EcoBase *Base = unitToBase[workerUnit];
    if (Base) {
        Base->removeMiner(workerUnit);
        unitToBase.erase(workerUnit);
        --workerAmount;
    }
}

void ResourceSupplier::removeMineral(BWAPI::Unit mineralUnit)
{
    EcoBase *Base = unitToBase[mineralUnit];
    if (Base) {
        Base->removeMineral(mineralUnit);
        unitToBase.erase(mineralUnit);
        --mineralAmount;
    }
}

bool ResourceSupplier::canFillLackingMiners() {
    BWAPI::Unit centerUnit = nullptr;
    for (EcoBase *Base: Bases) {
        if (Utils::isIdle(Base->getCenter()) && Base->isLackingMiners())
            return true;
    }
    return false;
}

void ResourceSupplier::createWorker()
{
    BWAPI::Unit centerUnit = nullptr;
    for (EcoBase *Base: Bases) {
        centerUnit = Base->getCenter();
        if (Utils::isIdle(centerUnit) && Base->isLackingMiners()) {
            centerUnit->train(workerType);
        }
    }
}

bool ResourceSupplier::isAtCapacity()
{
    for (EcoBase *Base: Bases) {
        if (Base->isLackingMiners()) {
            return false;
        }
    }
    return true;
}

void ResourceSupplier::displayStatus(int &row)
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
