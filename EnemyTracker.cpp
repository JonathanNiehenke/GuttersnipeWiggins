#pragma once
#include "EnemyTracker.h"

void EnemyTracker::addUnit(const BWAPI::Unit& unit) {
    visibleUnits.push_back(std::make_pair(
        unit, PositionalType(unit->getPosition(), unit->getType())));
}

void EnemyTracker::removeUnit(const BWAPI::Unit& unit) {
    visibleUnits.erase(
        std::remove_if(visibleUnits.begin(), visibleUnits.end(), 
            [unit](const UnitRecord& uR){ return uR.first == unit; }),
        visibleUnits.end());
}

void EnemyTracker::update() {
    discardVisible();
    updateVisible();
}

void EnemyTracker::discardVisible() {
    enemyPositions.erase(
        std::remove_if(enemyPositions.begin(), enemyPositions.end(), isVisible),
        enemyPositions.end());
}

bool EnemyTracker::isVisible(const PositionalType& posType) {
    return BWAPI::Broodwar->isVisible(BWAPI::TilePosition(posType.first));
}

void EnemyTracker::updateVisible() {
    for (UnitRecord& unitRecord: visibleUnits) {
        if (unitRecord.first->isVisible())
            updatePosition(unitRecord.second, unitRecord.first->getPosition());
    }
}

void EnemyTracker::updatePosition(
    PositionalType& positionaType, const BWAPI::Position& position)
{
    positionaType.first = position;
    enemyPositions.push_back(positionaType);
}

bool EnemyTracker::lacking(TypePred typePred) const {
    if (!typePred) return false;
    return !any_of(enemyPositions.begin(), enemyPositions.end(),
        [typePred](const PositionalType& posType)
            { return typePred(posType.second); });
}

BWAPI::Position EnemyTracker::closestTo(
    const BWAPI::Position& srcPos, TypePred typePred) const
{
    auto closer = Utils::Position(srcPos).comparePositions();
    auto closest = BWAPI::Positions::None;
    for (const PositionalType& positionalType: enemyPositions) {
        if (!closer(positionalType.first, closest)) continue;
        if (!typePred || typePred(positionalType.second))
            closest = positionalType.first;
    }
    return closest;
}

void EnemyTracker::drawStatus() const {
    for (const PositionalType& positionalType: enemyPositions) {
        BWAPI::Broodwar->drawTextMap(
            positionalType.first, positionalType.second.c_str());
    }
}
