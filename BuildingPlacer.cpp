#pragma once
#include "BuildingPlacer.h"

BuildingPlacer::BuildingPlacer(int columnWidth, int rowHeight)
    : columnWidth(columnWidth), rowHeight(rowHeight),
      srcLocation(BWAPI::Broodwar->self()->getStartLocation())
{}

BWAPI::TilePosition BuildingPlacer::getPlacement(
    const BWAPI::UnitType& buildingType) const
{
    bool small = buildingType.tileSize().y == 2;
    const BWAPI::TilePosition& startPos = BWAPI::Broodwar->self()->getStartLocation();
    for (int radius = 0; radius < 4; ++radius) {
        for (const BWAPI::TilePosition& offset: RadicalOffset(radius)) {
            BWAPI::TilePosition buildLocation = startPos + BWAPI::TilePosition(
                offset.x * columnWidth, offset.y * rowHeight);
            if (BWAPI::Broodwar->canBuildHere(buildLocation, buildingType))
                return buildLocation;
        }
    }
    return BWAPI::TilePositions::None;
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
