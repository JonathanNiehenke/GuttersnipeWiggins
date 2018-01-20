#pragma once
#include "BuildingConstructor.h"

void BuildingConstructor::onStart(const BWAPI::TilePosition& srcPosition) {
    this->srcPosition = srcPosition;
}

void BuildingConstructor::requestPreparation(
    const BWAPI::UnitType& productType)
{
    if (inPreparation.find(productType) == inPreparation.end())
        assignPreparation(productType);
}

void BuildingConstructor::assignPreparation(
    const BWAPI::UnitType& productType)
{
    try {
        inPreparation[productType] = createJob(productType); }
    catch (const std::runtime_error& e) {
        BWAPI::Broodwar << e.what(); }
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
        Job.productType, srcPosition, 25);
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
    
    for (const auto& prepPair: inPreparation) {
        ConstrunctionPO Job = prepPair.second;
        BWAPI::Broodwar->sendTextEx(true, "updating: %s", Job.productType.c_str());
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

void BuildingConstructor::promoteToProduction(
        const BWAPI::Unit& createdBuilding)
{
    BWAPI::UnitType createdType = createdBuilding->getType();
    try {
        ConstrunctionPO& Job = inPreparation.at(createdType);
        inPreparation.erase(createdType);
        Job.productType = createdType;
        inProduction.push_back(Job);
    }
    catch (std::out_of_range) {
        BWAPI::Broodwar << "No Job found associated with created building";
    }
}

void BuildingConstructor::setAsComplete(const BWAPI::Unit& completedBuilding) {
    auto it = inProduction.begin();
    for (; it != inProduction.end(); ++it) {
        if (it->product == completedBuilding) {
            inProduction.erase(it);
            return;
        }
    }
    BWAPI::Broodwar << "No Job found associated with completed building";
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
