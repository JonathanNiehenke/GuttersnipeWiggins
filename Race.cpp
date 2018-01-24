#pragma once
#include "Race.h"

void Race::onUnitCreate(const BWAPI::Unit& createdUnit) {
    if (createdUnit->getType().isBuilding())
        buildingConstructor->onCreate(createdUnit);
}

void Race::onUnitMorph(const BWAPI::Unit& morphedUnit) {
    BWAPI::Broodwar << "Unhandled morph: " << morphedUnit->getType().c_str();
}

void Race::onUnitComplete(const BWAPI::Unit& completedUnit) {
    if (completedUnit->getType().isBuilding())
        buildingConstructor->onComplete(createdUnit);
}

int Race::expectedSupplyProvided(const BWAPI::UnitType& providerType) const {
    const static int supplyProvided = providerType.supplyProvided();
    int providerCount = BWAPI::Broodwar->self()->allUnitCount(providerType);
    return supplyProvided * providerCount;
}

int Race::expectedSupplyProvided() const {
    const int defaultProvided = expectedSupplyProvided(supplyType),
              centerProvided = expectedSupplyProvided(centerType);
    return defaultProvided + centerProvided;
}

int Race::potentialSupplyUsed(const BWAPI::UnitType& unitType) const {
    const static float supplyBuildTime = float(supplyType.buildTime());
    const int unitsPerSupplyBuild = int(std::ceil(
             supplyBuildTime / unitType.buildTime()));
    const int facilityAmount = (unitType == workerType
        ? resourceSupplier->getBaseAmount()
        : armyTrainer->facilityCount(unitType));
    return unitsPerSupplyBuild * facilityAmount * unitType.supplyRequired();
}

void Race::createSupply() {
    buildingConstructor->request(supplyType);
}

void Race::createWorkers() {
    resourceSupplier->createWorker();
}

void Race::techTo(const BWAPI::UnitType& unitType) {
}

void trainArmyUnit(const BWAPI::UnitType& unitType) {
    armyTrainer->trainUnits(unitType);
}

void constructFacility(const BWAPI::UnitType& buildingType) {
    buildingConstructor->request(buildingType);
}

