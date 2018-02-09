#pragma once
#include "BuildingConstructor.h"

BuildingConstructor::BuildingConstructor() {
    this->srcLocation = BWAPI::Broodwar->self()->getStartLocation();
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
    const ConstrunctionPO& Job) const
{
    BWAPI::TilePosition placement = getPlacement(
        Job.productType);
    if (placement == BWAPI::TilePositions::None)
        throw std::runtime_error("No suitable placement found");
    return placement;
}

BWAPI::TilePosition BuildingConstructor::getPlacement(
    const BWAPI::UnitType& buildingType) const
{
    bool small = buildingType.tileSize().y == 2;
    const BWAPI::TilePosition& startPos = BWAPI::Broodwar->self()->getStartLocation();
    for (int radius = 0; radius < 4; ++radius) {
        for (const BWAPI::TilePosition& offset: RadicalOffset(radius)) {
            BWAPI::TilePosition buildLocation = startPos + offset * 5;
            if (BWAPI::Broodwar->canBuildHere(buildLocation, buildingType))
                return buildLocation;
            if (small && canBuildAdjacent(buildLocation, buildingType))
                return adjacentBuildLocation(buildLocation, buildingType);
        }
    }
    return BWAPI::TilePositions::None;
}

bool BuildingConstructor::canBuildAdjacent(
    const BWAPI::TilePosition& buildingLocation,
    const BWAPI::UnitType& buildingType)
{
    const BWAPI::TilePosition buildingSize = buildingType.tileSize();
    const BWAPI::Position buildingPos = BWAPI::Position(buildingLocation);
    const BWAPI::Unitset& adjacent = BWAPI::Broodwar->getUnitsInRectangle(
        buildingPos, buildingPos + BWAPI::Position(4*32, 4*32));
    auto isSimilarSize = (
        [buildingSize](const BWAPI::Unit& unit) -> bool {
            return unit->getType().tileSize() == buildingSize; });
    if (!std::all_of(adjacent.begin(), adjacent.end(), isSimilarSize))
        return false;
    if (adjacent.size() >= (buildingSize.x == 2 ? size_t(4) : size_t(2)))
        return false;
    return true;
}

BWAPI::TilePosition BuildingConstructor::adjacentBuildLocation(
    const BWAPI::TilePosition& buildingLocation,
    const BWAPI::UnitType& buildingType)
{
    BWAPI::TilePosition& newBuildingLoc = (
        buildingLocation + BWAPI::TilePosition(0, 2));
    if (BWAPI::Broodwar->canBuildHere(newBuildingLoc, buildingType))
        return newBuildingLoc;
    newBuildingLoc = buildingLocation + BWAPI::TilePosition(2, 0);
    if (BWAPI::Broodwar->canBuildHere(newBuildingLoc, buildingType))
        return newBuildingLoc;
    return buildingLocation + BWAPI::TilePosition(2, 2);
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


BuildingConstructor::RadicalOffset::iterator
    BuildingConstructor::RadicalOffset::begin()
{
    return iterator(radius, 0);
}

BuildingConstructor::RadicalOffset::iterator
    BuildingConstructor::RadicalOffset::end()
{
    return iterator(radius, radius * 8);
}

void BuildingConstructor::RadicalOffset::iterator::operator++() {
    ++count;
}

bool BuildingConstructor::RadicalOffset::iterator::operator==(
    iterator other) const
{
    return radius == other.radius && count == other.count;
}

bool BuildingConstructor::RadicalOffset::iterator::operator!=(
    iterator other) const
{
    return !(*this == other);
}

BWAPI::TilePosition BuildingConstructor::RadicalOffset::iterator::operator*()
    const
{
    if (count <= radius*2)
        return BWAPI::TilePosition(count - radius, -radius);
    if (count <= radius*4)
        return BWAPI::TilePosition(radius, count - radius*3);
    if (count <= radius*6)
        return BWAPI::TilePosition(radius*5 - count, radius);
    return BWAPI::TilePosition(-radius, radius*7 - count);
}
