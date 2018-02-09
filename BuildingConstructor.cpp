#pragma once
#include "BuildingConstructor.h"

BuildingConstructor::BuildingConstructor() {
    this->srcLocation = BWAPI::Broodwar->self()->getStartLocation();
    buildingPlacer = new BuildingPlacer();
}

BuildingConstructor::~BuildingConstructor() {
    delete buildingPlacer;
}

void BuildingConstructor::request(
    const BWAPI::UnitType& productType)
{
    if (Preparing.find(productType) == Preparing.end())
        beginPreparation(productType);
}

void BuildingConstructor::beginPreparation(
    const BWAPI::UnitType& productType)
{
    ConstrunctionPO Job(productType);
    Job.placement = buildingPlacer->getPlacement(Job.productType);
    Job.contractor = getContractor(Job);
    Preparing[productType] = Job;
}

BWAPI::Unit BuildingConstructor::getContractor(
    const ConstrunctionPO& Job)
{
    if (Job.placement == BWAPI::TilePositions::None) return nullptr;
    // IsGatheringMinerals assumes IsWorker && IsOwned
    return BWAPI::Broodwar->getClosestUnit(toJobCenter(Job),
        IsGatheringMinerals && CurrentOrder == BWAPI::Orders::MoveToMinerals);
}

BWAPI::Position BuildingConstructor::toJobCenter(const ConstrunctionPO& Job) {
    return (BWAPI::Position(Job.placement) + BWAPI::Position(
        Job.productType.tileSize()) / 2);
}

void BuildingConstructor::updatePreparation() {
    for (auto& prepPair: Preparing) {
        ConstrunctionPO& Job = prepPair.second;
        if (Job.placement == BWAPI::TilePositions::None)
            Job.placement = buildingPlacer->getPlacement(Job.productType);
        else if (!Job.contractor)
            Job.contractor = getContractor(Job);
        else if (isPrepared(Job))
            construct(Job);
        else if (Job.contractor->isGatheringMinerals())
            Job.contractor->move(toJobCenter(Job));
    }
}

bool BuildingConstructor::isPrepared(const ConstrunctionPO& Job) {
    BWAPI::Position contractorPos = Job.contractor->getPosition();
    return (contractorPos.getApproxDistance(toJobCenter(Job)) < 64 &&
            Job.contractor->canBuild(Job.productType, Job.placement));
}

void BuildingConstructor::construct(const ConstrunctionPO& Job) {
    // Reduces checks at completion by queueing the return to mining
    if (Job.contractor->build(Job.productType, Job.placement))
        queueReturnToMining(Job.contractor);
}

void BuildingConstructor::queueReturnToMining(const BWAPI::Unit& worker) {
    BWAPI::Unit closestMineral = BWAPI::Broodwar->getClosestUnit(
        worker->getPosition(), IsMineralField);
    worker->gather(closestMineral, true);
}

void BuildingConstructor::onCreate(const BWAPI::Unit& createdBuilding) {
    try {
        ConstrunctionPO& Job = Preparing.at(createdBuilding->getType());
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

BuildingConstructor::ConstrunctionPO::ConstrunctionPO() {
    this->productType = BWAPI::UnitTypes::None;
    this->placement = BWAPI::TilePositions::None;
    this->contractor = nullptr;
    this->product = nullptr;
};

BuildingConstructor::ConstrunctionPO::ConstrunctionPO(const BWAPI::UnitType& productType) {
    this->productType = productType;
    this->placement = BWAPI::TilePositions::None;
    this->contractor = nullptr;
    this->product = nullptr;
};
