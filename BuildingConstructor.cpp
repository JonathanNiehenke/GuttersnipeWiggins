#pragma once
#include "BuildingConstructor.h"

BuildingConstructor::BuildingConstructor(BuildingPlacer* buildingPlacer)
    : srcLocation(BWAPI::Broodwar->self()->getStartLocation()),
      buildingPlacer(buildingPlacer)
{}

BuildingConstructor::~BuildingConstructor() {
    delete buildingPlacer;
}

void BuildingConstructor::request(const BWAPI::UnitType& productType) {
    if (Preparing.find(productType) != Preparing.end()) return;
    ConstructionPO Job(productType);
    beginConstructionPreparation(Job);
    Preparing[productType] = Job;
}

void BuildingConstructor::beginConstructionPreparation(
    ConstructionPO& Job) const
{
    Job.placement = (Job.productType.isRefinery()
        ? buildingPlacer->getGasPlacement()
        : buildingPlacer->getPlacement(Job.productType));
    Job.contractor = getContractor(Job);
}

BWAPI::Unit BuildingConstructor::getContractor(
    const ConstructionPO& Job) const
{
    if (Job.placement == BWAPI::TilePositions::None) return nullptr;
    // IsGatheringMinerals assumes IsWorker && IsOwned
    return BWAPI::Broodwar->getClosestUnit(toJobCenter(Job),
        IsGatheringMinerals && CurrentOrder == BWAPI::Orders::MoveToMinerals);
}

BWAPI::Position BuildingConstructor::toJobCenter(const ConstructionPO& Job) {
    return (BWAPI::Position(Job.placement) + BWAPI::Position(
        Job.productType.tileSize()) / 2);
}

void BuildingConstructor::updatePreparation() {
    for (auto& prepPair: Preparing) {
        ConstructionPO& Job = prepPair.second;
        if (Job.placement == BWAPI::TilePositions::None)
            Job.placement = buildingPlacer->getPlacement(Job.productType);
        else if (!Job.contractor)
            Job.contractor = getContractor(Job);
        else if (isPrepared(Job))
            construct(Job);
        else if (!isPreparing(Job))
            Job.contractor->move(toJobCenter(Job));
    }
}

bool BuildingConstructor::isPrepared(const ConstructionPO& Job) {
    const BWAPI::Position& contractorPos = Job.contractor->getPosition();
    return (contractorPos.getApproxDistance(toJobCenter(Job)) < 90 &&
            Job.contractor->canBuild(Job.productType));
}

void BuildingConstructor::construct(ConstructionPO& Job) {
    if (Job.contractor->build(Job.productType, Job.placement))
        queueReturnToMining(Job.contractor);
    else if (isObstructed(Job))
        Job.placement = buildingPlacer->getPlacement(Job.productType);
}

void BuildingConstructor::queueReturnToMining(const BWAPI::Unit& worker) {
    BWAPI::Unit closestMineral = BWAPI::Broodwar->getClosestUnit(
        worker->getPosition(), IsMineralField);
    worker->gather(closestMineral, true);
}

bool BuildingConstructor::isObstructed(const ConstructionPO& Job) {
    return !BWAPI::Broodwar->getUnitsInRectangle(
        BWAPI::Position(Job.placement),
        BWAPI::Position(Job.placement + Job.productType.tileSize()),
        IsBuilding || IsEnemy || !IsMoving).empty();
}

bool BuildingConstructor::isPreparing(const ConstructionPO& Job) {
    return (Job.contractor->isConstructing() ||
            Job.contractor->getTargetPosition() == toJobCenter(Job));
}

void BuildingConstructor::onCreate(const BWAPI::Unit& createdBuilding) {
    try {
        ConstructionPO& Job = Preparing.at(createdBuilding->getType());
        Preparing.erase(Job.productType);
        Job.product = createdBuilding;
        Producing[createdBuilding] = Job;
    }
    catch (std::out_of_range) {
        BWAPI::Broodwar << "No Job found associated with created building"
                        << std::endl;
    }
}

void BuildingConstructor::onComplete(const BWAPI::Unit& completedBuilding) {
    Producing.erase(completedBuilding);
}

MorphingConstructor::MorphingConstructor()
    : BuildingConstructor(new BuildingPlacer(5, 4)) {}

void MorphingConstructor::request(const BWAPI::UnitType& productType) {
    if (Preparing.find(productType) != Preparing.end()) return;
    ConstructionPO Job(productType);
    if (productType.whatBuilds().first.isWorker())
        beginConstructionPreparation(Job);
    else
        beginMorphingPreparation(Job);
    Preparing[productType] = Job;
}

void MorphingConstructor::beginMorphingPreparation(ConstructionPO& Job) const {
    const BWAPI::Unit& zergBuilding = BWAPI::Broodwar->getClosestUnit(
        BWAPI::Position(srcLocation),
        GetType == Job.productType.whatBuilds().first,
        64);
    if (!zergBuilding) return;
    Job.placement = zergBuilding->getTilePosition();
    Job.contractor = zergBuilding;
}

void MorphingConstructor::updatePreparation() {
    for (auto& prepPair: Preparing) {
        ConstructionPO& Job = prepPair.second;
        if (Job.placement == BWAPI::TilePositions::None)
            Job.placement = buildingPlacer->getPlacement(Job.productType);
        else if (!Job.contractor)
            Job.contractor = getContractor(Job);
        else if (Job.contractor->getType().isBuilding())
            Job.contractor->morph(Job.productType);
        else if (isPrepared(Job))
            construct(Job);
        else if (!isPreparing(Job))
            Job.contractor->move(toJobCenter(Job));
    }
}

AddonConstructor::AddonConstructor()
    : BuildingConstructor(new AddonPlacer()) {}

void AddonConstructor::request(const BWAPI::UnitType& productType) {
    if (Preparing.find(productType) != Preparing.end()) return;
    ConstructionPO Job(productType);
    if (productType.isAddon())
        beginAddonPreparation(Job);
    else
        beginConstructionPreparation(Job);
    Preparing[productType] = Job;
}

void AddonConstructor::beginAddonPreparation(ConstructionPO& Job) const {
    const BWAPI::UnitType& contractorType = Job.productType.whatBuilds().first;
    const BWAPI::Unit& terranBuilding = BWAPI::Broodwar->getClosestUnit(
        BWAPI::Position(srcLocation), GetType == contractorType, 300);
    if (!terranBuilding) return;
    Job.placement = terranBuilding->getTilePosition();
    Job.contractor = terranBuilding;
}

void AddonConstructor::updatePreparation() {
    for (auto& prepPair: Preparing) {
        ConstructionPO& Job = prepPair.second;
        if (Job.placement == BWAPI::TilePositions::None)
            Job.placement = buildingPlacer->getPlacement(Job.productType);
        else if (!Job.contractor)
            Job.contractor = getContractor(Job);
        else if (Job.productType.isAddon()) {
            BWAPI::Broodwar->sendTextEx(true, "%s (%d, %d) - %s",
                Job.productType.c_str(), Job.placement.x, Job.placement.y,
                Job.contractor->getType().c_str());
            Job.contractor->buildAddon(Job.productType);
        }
        else if (isPrepared(Job))
            construct(Job);
        else if (!isPreparing(Job))
            Job.contractor->move(toJobCenter(Job));
    }
}

BuildingConstructor::ConstructionPO::ConstructionPO() {
    this->productType = BWAPI::UnitTypes::None;
    this->placement = BWAPI::TilePositions::None;
    this->contractor = nullptr;
    this->product = nullptr;
};

BuildingConstructor::ConstructionPO::ConstructionPO(const BWAPI::UnitType& productType) {
    this->productType = productType;
    this->placement = BWAPI::TilePositions::None;
    this->contractor = nullptr;
    this->product = nullptr;
};
