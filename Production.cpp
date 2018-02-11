#pragma once
#include "Production.h"

Production::Production(const BWAPI::UnitType& armyUnitType) {
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

Production::~Production() {
    delete buildingConstructor;
    delete resourceSupplier;
    delete unitTrainer;
    delete techTree;
}

void Production::onUnitCreate(const BWAPI::Unit& createdUnit) const {
    if (createdUnit->getType().isBuilding())
        buildingConstructor->onCreate(createdUnit);
}

void Production::onUnitMorph(const BWAPI::Unit& morphedUnit) {
    BWAPI::Broodwar << "Unhandled morph: " << morphedUnit->getType().c_str()
                    << std::endl;
}

void Production::onUnitComplete(const BWAPI::Unit& completedUnit) {
    if (completedUnit->getType() == workerType)
        resourceSupplier->addWorker(completedUnit);
    else if (completedUnit->getType().isBuilding())
        onCompleteBuilding(completedUnit);
}

void Production::onCompleteBuilding(
    const BWAPI::Unit& completedBuilding) const
{
    buildingConstructor->onComplete(completedBuilding);
    techTree->addTech(completedBuilding->getType());
    if (completedBuilding->getType().canProduce())
        unitTrainer->includeFacility(completedBuilding);
    if (completedBuilding->getType() == centerType)
        resourceSupplier->addBase(completedBuilding);
}

void Production::onUnitDestroy(const BWAPI::Unit& destroyedUnit) const {
    if (destroyedUnit->getType() == workerType)
        resourceSupplier->removeWorker(destroyedUnit);
    else if (destroyedUnit->getType().isBuilding())
        onDestroyedBuilding(destroyedUnit);
}

void Production::onDestroyedBuilding(
    const BWAPI::Unit& destroyedBuilding) const
{
    buildingConstructor->onComplete(destroyedBuilding);
    if (destroyedBuilding->getType().canProduce())
        unitTrainer->removeFacility(destroyedBuilding);
    if (destroyedBuilding->getType() == centerType)
        resourceSupplier->removeBase(destroyedBuilding);
}

void Production::update() const {
    buildingConstructor->updatePreparation();
}

int Production::expectedSupplyProvided(
    const BWAPI::UnitType& providerType) const
{
    const int providerCount = BWAPI::Broodwar->self()->allUnitCount(
        providerType);
    return providerType.supplyProvided() * providerCount;
}

int Production::expectedSupplyProvided() const {
    const int defaultProvided = expectedSupplyProvided(supplyType),
              centerProvided = expectedSupplyProvided(centerType);
    return defaultProvided + centerProvided;
}

int Production::potentialSupplyUsed(const BWAPI::UnitType& unitType) const {
    const static float supplyBuildTime = float(supplyType.buildTime());
    const int unitsPerSupplyBuild = int(std::ceil(
             supplyBuildTime / unitType.buildTime()));
    const int facilityAmount = (unitType == workerType
        ? resourceSupplier->getBaseAmount()
        : unitTrainer->facilityCount());
    return unitsPerSupplyBuild * facilityAmount * unitType.supplyRequired();
}

void Production::createSupply() const {
    buildingConstructor->request(supplyType);
}

bool Production::canFillLackingMiners() const {
    return resourceSupplier->canFillLackingMiners();
}

void Production::createWorker() const {
    unitTrainer->trainUnits(workerType);
}

bool Production::readyToTrainArmyUnit() const {
    return unitTrainer->readyToTrain(armyUnitType);
}

void Production::trainWarriors() const {
    unitTrainer->trainUnits(armyUnitType);
}

void Production::construct(const BWAPI::UnitType& buildingType) const {
    buildingConstructor->request(buildingType);
}

BWAPI::UnitType Production::getNextRequiredBuilding(
    const BWAPI::UnitType& unitType) const
{
    return techTree->getNextRequiredBuilding(unitType);
}

void ProtossProduction::construct(const BWAPI::UnitType& buildingType) const {
        buildingConstructor->request(buildingType);
}

BWAPI::UnitType ProtossProduction::getNextRequiredBuilding(
    const BWAPI::UnitType& unitType) const
{
    const auto& requiredType = Production::getNextRequiredBuilding(unitType);
    return (requiredType.requiresPsi() && !doesPylonExist()
        ? BWAPI::UnitTypes::Protoss_Pylon : requiredType);
}

bool ProtossProduction::doesPylonExist() const {
    return BWAPI::Broodwar->self()->allUnitCount(supplyType) > 0;
}

void ZergProduction::onUnitMorph(const BWAPI::Unit& morphedUnit) {
    if (isIncompleteOverlord(morphedUnit))
        ++incompleteOverlordCount;
    onUnitCreate(morphedUnit);
}

bool ZergProduction::isIncompleteOverlord(const BWAPI::Unit& unit) {
    return (unit->getType() == BWAPI::UnitTypes::Zerg_Egg &&
        unit->getBuildType() == BWAPI::UnitTypes::Zerg_Overlord);
}

void ZergProduction::onUnitComplete(const BWAPI::Unit& completedUnit) {
    if (completedUnit->getType() == supplyType && incompleteOverlordCount > 0)
        --incompleteOverlordCount;
    Production::onUnitComplete(completedUnit);
}

int ZergProduction::expectedSupplyProvided(
    const BWAPI::UnitType& providerType) const
{
    int providerCount = BWAPI::Broodwar->self()->allUnitCount(
        providerType);
    if (providerType == supplyType)
       providerCount  += incompleteOverlordCount;
    return providerType.supplyProvided() * providerCount;
}

void ZergProduction::createSupply() const {
    unitTrainer->trainUnits(supplyType);
}

void ZergProduction::construct(const BWAPI::UnitType& buildingType) const {
    if (buildingType == centerType || doesTechExist(buildingType))
        buildingConstructor->request(centerType);
    else
        buildingConstructor->request(buildingType);
}

bool ZergProduction::doesTechExist(const BWAPI::UnitType& buildingType) const {
    return BWAPI::Broodwar->self()->allUnitCount(buildingType) > 0;
}
