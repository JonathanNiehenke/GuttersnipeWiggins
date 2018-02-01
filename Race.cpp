#pragma once
#include "Race.h"

Race::Race(const BWAPI::UnitType& armyUnitType) {
    this->resourceSupplier = new ResourceSupplier(workerType);
    this->buildingConstructor = new BuildingConstructor();
    this->armyTrainer = new ArmyTrainer();
    this->techTree = new TechTree();
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
    delete techTree;
}

void Race::onUnitCreate(const BWAPI::Unit& createdUnit) const {
    if (createdUnit->getType().isBuilding())
        buildingConstructor->onCreate(createdUnit);
}

void Race::onUnitMorph(const BWAPI::Unit& morphedUnit) const {
    BWAPI::Broodwar << "Unhandled morph: " << morphedUnit->getType().c_str()
                    << std::endl;
}

void Race::onUnitComplete(const BWAPI::Unit& completedUnit) const {
    BWAPI::UnitType completedType = completedUnit->getType();
    if (completedType == workerType)
        resourceSupplier->addWorker(completedUnit);
    else if (completedType.isBuilding())
        onCompleteBuilding(completedUnit);
}

void Race::onCompleteBuilding(const BWAPI::Unit& completedBuilding) const {
    buildingConstructor->onComplete(completedBuilding);
    techTree->addTech(completedBuilding->getType());
    if (completedBuilding->getType() == centerType)
        resourceSupplier->addBase(completedBuilding);
}

void Race::onUnitDestroy(const BWAPI::Unit& destroyedUnit) const {
    BWAPI::UnitType destroyedType = destroyedUnit->getType();
    if (destroyedType == workerType)
        resourceSupplier->removeWorker(destroyedUnit);
    else if (destroyedType == centerType)
        resourceSupplier->removeBase(destroyedUnit);
    else if (destroyedType.isBuilding())
        onDestroyedBuilding(destroyedUnit);
}

void Race::update() const {
    buildingConstructor->updatePreparation();
}

int Race::expectedSupplyProvided(const BWAPI::UnitType& providerType) const {
    const int supplyProvided = providerType.supplyProvided();
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

void Race::createSupply() const {
    buildingConstructor->request(supplyType);
}

void Race::createWorker() const {
    resourceSupplier->createWorker();
}

bool Race::readyToTrainArmyUnit() const {
    return armyTrainer->readyToTrain();
}

void Race::trainWarriors() const {
    armyTrainer->trainUnits(armyUnitType);
}

void Race::construct(const BWAPI::UnitType& buildingType) const {
    buildingConstructor->request(buildingType);
}

BWAPI::UnitType Race::getNextRequiredBuilding(
    const BWAPI::UnitType& unitType) const
{
    return techTree->getNextRequiredBuilding(unitType);
}
