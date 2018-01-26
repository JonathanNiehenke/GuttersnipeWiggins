#pragma once
#include "Race.h"

Race::Race(const BWAPI::UnitType& armyUnitType) {
    this->resourceSupplier = new ResourceSupplier(workerType);
    this->buildingConstructor = new BuildingConstructor();
    this->armyTrainer = new ArmyTrainer();
    const BWAPI::Race& r = BWAPI::Broodwar->self()->getRace();
    this->centerType = r.getCenter();
    this->workerType = r.getWorker();
    this->supplyType = r.getSupplyProvider();
    this->armyUnitType = armyUnitType;
}

Race::~Race() {
    delete buildingConstructor;
    delete resourceSupplier;
    delete armyTrainer;
}

void Race::onUnitCreate(const BWAPI::Unit& createdUnit) {
    if (createdUnit->getType().isBuilding())
        buildingConstructor->onCreate(createdUnit);
}

void Race::onUnitMorph(const BWAPI::Unit& morphedUnit) {
    BWAPI::Broodwar << "Unhandled morph: " << morphedUnit->getType().c_str();
}

void Race::onUnitComplete(const BWAPI::Unit& completedUnit) {
    BWAPI::UnitType completedType = completedUnit->getType();
    if (completedType.isBuilding())
        buildingConstructor->onComplete(completedUnit);
}

void Race::onUnitDestroy(const BWAPI::Unit& destroyedUnit) {
    BWAPI::UnitType destroyedType = destroyedUnit->getType();
    if (destroyedType == workerType)
        resourceSupplier->removeWorker(destroyedUnit);
    else if (destroyedType == centerType)
        resourceSupplier->removeBase(destroyedUnit);
    else if (destroyedType.isBuilding())
        onDestroyedBuilding(destroyedUnit);
}

void Race::update() {
    buildingConstructor->updatePreparation();
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
        : armyTrainer->facilityCount());
    return unitsPerSupplyBuild * facilityAmount * unitType.supplyRequired();
}

void Race::createSupply() {
    buildingConstructor->request(supplyType);
}

void Race::createWorker() {
    resourceSupplier->createWorker();
}

bool Race::readyToTrainArmyUnit() {
    return armyTrainer->readyToTrain();
}

void Race::trainWarriors() {
    armyTrainer->trainUnits(armyUnitType);
}

void Race::construct(const BWAPI::UnitType& buildingType) {
    buildingConstructor->request(buildingType);
}

