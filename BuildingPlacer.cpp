#pragma once
#include "BuildingPlacer.h"

BWAPI::TilePosition BuildingPlacer::getPlacement(
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

bool BuildingPlacer::canBuildAdjacent(
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

BWAPI::TilePosition BuildingPlacer::adjacentBuildLocation(
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

BWAPI::TilePosition BuildingPlacer::getGasPlacement() const {
    const BWAPI::TilePosition& startPos = BWAPI::Broodwar->self()->getStartLocation();
    const BWAPI::Unit& vespeneUnit = BWAPI::Broodwar->getClosestUnit(
        BWAPI::Position(startPos), BWAPI::Filter::GetType == BWAPI::UnitTypes::Resource_Vespene_Geyser, 200);
    if (vespeneUnit)
        return vespeneUnit->getTilePosition();
    return BWAPI::TilePositions::None;
}

BuildingPlacer::RadicalOffset::iterator
    BuildingPlacer::RadicalOffset::begin()
{
    return iterator(radius, 0);
}

BuildingPlacer::RadicalOffset::iterator
    BuildingPlacer::RadicalOffset::end()
{
    return iterator(radius, radius * 8);
}

void BuildingPlacer::RadicalOffset::iterator::operator++() {
    ++count;
}

bool BuildingPlacer::RadicalOffset::iterator::operator==(
    iterator other) const
{
    return radius == other.radius && count == other.count;
}

bool BuildingPlacer::RadicalOffset::iterator::operator!=(
    iterator other) const
{
    return !(*this == other);
}

BWAPI::TilePosition BuildingPlacer::RadicalOffset::iterator::operator*()
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
