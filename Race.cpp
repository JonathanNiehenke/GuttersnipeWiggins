#pragma once
#include "Race.h"

Race::Race(const BWAPI::UnitType& armyUnitType) {
    const BWAPI::Race& r = BWAPI::Broodwar->self()->getRace();
    this->centerType = r.getCenter();
    this->workerType = r.getWorker();
    this->supplyType = r.getSupplyProvider();
    this->armyUnitType = armyUnitType;
    this->resourceSupplier = new ResourceSupplier(workerType);
    this->buildingConstructor = new BuildingConstructor();
    this->unitTrainer = new UnitTrainer();
    this->techTree = new TechTree();
}

Race::~Race() {
    delete buildingConstructor;
    delete resourceSupplier;
    delete unitTrainer;
    delete techTree;
}

void Race::onUnitCreate(const BWAPI::Unit& createdUnit) const {
    if (createdUnit->getType().isBuilding())
        buildingConstructor->onCreate(createdUnit);
}

void Race::onUnitMorph(const BWAPI::Unit& morphedUnit) {
    BWAPI::Broodwar << "Unhandled morph: " << morphedUnit->getType().c_str()
                    << std::endl;
}

void Race::onUnitComplete(const BWAPI::Unit& completedUnit) {
    if (completedUnit->getType() == workerType)
        resourceSupplier->addWorker(completedUnit);
    else if (completedUnit->getType().isBuilding())
        onCompleteBuilding(completedUnit);
}

void Race::onCompleteBuilding(const BWAPI::Unit& completedBuilding) const {
    buildingConstructor->onComplete(completedBuilding);
    techTree->addTech(completedBuilding->getType());
    if (completedBuilding->getType().canProduce())
        unitTrainer->includeFacility(completedBuilding);
    if (completedBuilding->getType() == centerType)
        resourceSupplier->addBase(completedBuilding);
}

void Race::onUnitDestroy(const BWAPI::Unit& destroyedUnit) const {
    if (destroyedUnit->getType() == workerType)
        resourceSupplier->removeWorker(destroyedUnit);
    else if (destroyedUnit->getType().isBuilding())
        onDestroyedBuilding(destroyedUnit);
}

void Race::onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding) const {
    buildingConstructor->onComplete(destroyedBuilding);
    if (destroyedBuilding->getType().canProduce())
        unitTrainer->removeFacility(destroyedBuilding);
    if (destroyedBuilding->getType() == centerType)
        resourceSupplier->removeBase(destroyedBuilding);
}

void Race::update() const {
    buildingConstructor->updatePreparation();
}

int Race::expectedSupplyProvided(const BWAPI::UnitType& providerType) const {
    const int providerCount = BWAPI::Broodwar->self()->allUnitCount(
        providerType);
    return providerType.supplyProvided() * providerCount;
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
        : unitTrainer->facilityCount());
    return unitsPerSupplyBuild * facilityAmount * unitType.supplyRequired();
}

void Race::createSupply() const {
    buildingConstructor->request(supplyType);
}

bool Race::canFillLackingMiners() const {
    return resourceSupplier->canFillLackingMiners();
}

void Race::createWorker() const {
    unitTrainer->trainUnits(workerType);
}

bool Race::readyToTrainArmyUnit() const {
    return unitTrainer->readyToTrain(armyUnitType);
}

void Race::trainWarriors() const {
    unitTrainer->trainUnits(armyUnitType);
}

void Race::construct(const BWAPI::UnitType& buildingType) const {
    buildingConstructor->request(buildingType);
}

BWAPI::UnitType Race::getNextRequiredBuilding(
    const BWAPI::UnitType& unitType) const
{
    return techTree->getNextRequiredBuilding(unitType);
}

void ProtossRace::construct(const BWAPI::UnitType& buildingType) const {
        buildingConstructor->request(buildingType);
}

BWAPI::UnitType ProtossRace::getNextRequiredBuilding(
    const BWAPI::UnitType& unitType) const
{
    const auto& requiredType = Race::getNextRequiredBuilding(unitType);
    return (requiredType.requiresPsi() && !doesPylonExist()
        ? BWAPI::UnitTypes::Protoss_Pylon : requiredType);
}

bool ProtossRace::doesPylonExist() const {
    return BWAPI::Broodwar->self()->allUnitCount(supplyType) > 0;
}

void ZergRace::onUnitMorph(const BWAPI::Unit& morphedUnit) {
    if (isIncompleteOverlord(morphedUnit))
        ++incompleteOverlordCount;
    onUnitCreate(morphedUnit);
}

bool ZergRace::isIncompleteOverlord(const BWAPI::Unit& unit) {
    return (unit->getType() == BWAPI::UnitTypes::Zerg_Egg &&
        unit->getBuildType() == BWAPI::UnitTypes::Zerg_Overlord);
}

void ZergRace::onUnitComplete(const BWAPI::Unit& completedUnit) {
    if (completedUnit->getType() == supplyType && incompleteOverlordCount > 0)
        --incompleteOverlordCount;
    Race::onUnitComplete(completedUnit);
}

int ZergRace::expectedSupplyProvided(
    const BWAPI::UnitType& providerType) const
{
    int providerCount = BWAPI::Broodwar->self()->allUnitCount(
        providerType);
    if (providerType == supplyType)
       providerCount  += incompleteOverlordCount;
    return providerType.supplyProvided() * providerCount;
}

void ZergRace::createSupply() const {
    unitTrainer->trainUnits(supplyType);
}

void ZergRace::construct(const BWAPI::UnitType& buildingType) const {
    if (buildingType == centerType || doesTechExist(buildingType))
        buildingConstructor->request(centerType);
    else
        buildingConstructor->request(buildingType);
}

bool ZergRace::doesTechExist(const BWAPI::UnitType& buildingType) const {
    return BWAPI::Broodwar->self()->allUnitCount(buildingType) > 0;
}
