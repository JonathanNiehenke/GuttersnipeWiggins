#pragma once
#include "Utils.h"

const BWAPI::Position Utils::Position::toTileCenter = BWAPI::Position(16, 16);

Utils::Position::Position() {
    srcPosition = BWAPI::Positions::None;
}

Utils::Position::Position(BWAPI::Position position) {
    srcPosition = position;
}

Utils::Position Utils::Position::fromLocation(BWAPI::TilePosition location) {
    return Utils::Position(BWAPI::Position(location) + toTileCenter);
}

Utils::Position Utils::Position::fromUnit(BWAPI::Unit unit) {
    return Utils::Position(unit->getPosition());
}

Utils::Position Utils::Position::fromUnitset(BWAPI::Unitset unitset) {
    return Utils::Position(unitset.getPosition());
}

int Utils::Position::operator-(BWAPI::Position pos) {
    return srcPosition.getApproxDistance(pos);
}

int Utils::Position::operator-(BWAPI::TilePosition loc) {
    return srcPosition.getApproxDistance(BWAPI::Position(loc) + toTileCenter);
}

int Utils::Position::operator-(BWAPI::Unit unit) {
    return srcPosition.getApproxDistance(unit->getPosition());
}

int Utils::Position::operator-(BWAPI::Unitset unitset) {
    return srcPosition.getApproxDistance(unitset.getPosition());
}

BWAPI::Position Utils::Position::get() {
    return srcPosition;
}

std::function<bool (BWAPI::Position, BWAPI::Position)>
    Utils::Position::comparePositions()
{
    return [this] (BWAPI::Position pos1, BWAPI::Position pos2)
        { return (*this) - pos1 < (*this) - pos2; };
}

std::function<bool (BWAPI::TilePosition, BWAPI::TilePosition)>
    Utils::Position::compareLocations()
{
    return [this] (BWAPI::TilePosition loc1, BWAPI::TilePosition loc2)
        { return (*this) - loc1 < (*this) - loc2; };
}

std::function<bool (BWAPI::Unit, BWAPI::Unit)> Utils::Position::compareUnits() {
    return [this] (BWAPI::Unit unit1, BWAPI::Unit unit2)
        { return (*this) - unit1 < (*this) - unit2; };
}

std::function<bool (BWAPI::Unitset, BWAPI::Unitset)>
    Utils::Position::compareUnitsets()
{
    return [this] (BWAPI::Unitset unitset1, BWAPI::Unitset unitset2)
        { return (*this) - unitset1 < (*this) - unitset2; };
}

bool Utils::isIdle(BWAPI::Unit Facility) {
    // Zerg hatchery is always idle so determine with larva.
    // Negate larva for Protoss and Terran with producesLarva.
    return (Facility->isIdle() && !(Facility->getType().producesLarva() &&
            Facility->getLarva().empty()));
}
