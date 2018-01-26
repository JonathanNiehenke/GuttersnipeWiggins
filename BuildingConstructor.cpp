#pragma once
#include "BuildingConstructor.h"

void BuildingConstructor::setSourceLocation(const BWAPI::TilePosition& srcLocation) {
    this->srcLocation = srcLocation;
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
    try {
        Preparing[productType] = createJob(productType); }
    catch (const std::runtime_error& e) {
        BWAPI::Broodwar << e.what() << std::endl; }
}

BuildingConstructor::ConstrunctionPO BuildingConstructor::createJob(
    const BWAPI::UnitType& productType)
{
    ConstrunctionPO Job(productType);
    Job.placement = getPlacement(Job);
    Job.contractor = getContractor(Job);
    return Job;
}

BWAPI::TilePosition BuildingConstructor::getPlacement(
    const ConstrunctionPO& Job)
{
    BWAPI::TilePosition placement = BWAPI::Broodwar->getBuildLocation(
        Job.productType, srcLocation, 25);
    if (placement == BWAPI::TilePositions::Invalid)
        throw std::runtime_error("No suitable placement found");
    return placement;
}

BWAPI::Unit BuildingConstructor::getContractor(
    const ConstrunctionPO& Job)
{
    // IsGatheringMinerals assumes IsWorker && IsOwned
    BWAPI::Unit contractor = BWAPI::Broodwar->getClosestUnit(toJobCenter(Job),
        IsGatheringMinerals && CurrentOrder == BWAPI::Orders::MoveToMinerals);
    if (!contractor)
        throw std::runtime_error("No suitable contractor found");
    return contractor;
}

BWAPI::Position BuildingConstructor::toJobCenter(const ConstrunctionPO& Job) {
    return (BWAPI::Position(Job.placement) + BWAPI::Position(
        Job.productType.tileSize()) / 2);
}

void BuildingConstructor::updatePreparation() {
    for (const auto& prepPair: Preparing) {
        ConstrunctionPO Job = prepPair.second;
        if (isPrepared(Job))
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
