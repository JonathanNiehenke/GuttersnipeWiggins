#pragma once
#include "EnemyPositions.h"

void EnemyPositions::include(const BWAPI::Unit& unit) {
    visibleUnits.push_back(std::make_pair(
        unit, PositionalType(unit->getPosition(), unit->getType())));
}

void EnemyPositions::discard(const BWAPI::Unit& unit) {
    visibleUnits.erase(
        std::remove_if(visibleUnits.begin(), visibleUnits.end(), 
            [unit](const UnitRecord& uR){ return uR.first == unit; }),
        visibleUnits.end());
}

void EnemyPositions::update() {
    discardVisible();
    updateVisible();
}

void EnemyPositions::discardVisible() {
    enemyPositions.erase(
        std::remove_if(enemyPositions.begin(), enemyPositions.end(), isVisible),
        enemyPositions.end());
}

bool EnemyPositions::isVisible(const PositionalType& posType) {
    return BWAPI::Broodwar->isVisible(BWAPI::TilePosition(posType.first));
}

void EnemyPositions::updateVisible() {
    for (UnitRecord& unitRecord: visibleUnits) {
        if (unitRecord.first->isVisible())
            updatePosition(unitRecord.second, unitRecord.first->getPosition());
    }
}

void EnemyPositions::updatePosition(
    PositionalType& positionaType, const BWAPI::Position& position)
{
    positionaType.first = position;
    enemyPositions.push_back(positionaType);
}

bool EnemyPositions::lacking(TypePred typePred) const {
    if (!typePred) return false;
    return !any_of(enemyPositions.begin(), enemyPositions.end(),
        [typePred](const PositionalType& posType)
            { return typePred(posType.second); });
}

BWAPI::Position EnemyPositions::closestTo(
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

void EnemyPositions::drawStatus() const {
    for (const PositionalType& positionalType: enemyPositions) {
        BWAPI::Broodwar->drawTextMap(
            positionalType.first, positionalType.second.c_str());
    }
}
