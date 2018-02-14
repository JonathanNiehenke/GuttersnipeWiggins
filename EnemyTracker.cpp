#pragma once
#include "EnemyTracker.h"

bool EnemyTracker::empty() const {
    return currentPositions.empty() && foggyPositions.empty();
}

void EnemyTracker::addUnit(const BWAPI::Unit& unit) {
    if (unit->isDetected()) {
        currentPositions[unit] = PositionalType(
            unit->getPosition(), unit->getType());
    }
}

void EnemyTracker::removeUnit(const BWAPI::Unit& unit) {
    currentPositions.erase(unit);
}

void EnemyTracker::update() {
    updateCurrentPositions();
    updateFoggyPositions();
}

void EnemyTracker::updateCurrentPositions() {
    for (auto& It = currentPositions.begin(); It != currentPositions.end();) {
        PositionalType& positionalType = It->second;
        if (It->first->isVisible())
            positionalType.first = (It++)->first->getPosition();
        else if (isInFog(positionalType.first))
            moveToFoggyPositions(*(It++));
        else
            ++It;
    }
}

void EnemyTracker::moveToFoggyPositions(
    const std::pair<BWAPI::Unit, PositionalType>& pair)
{
    foggyPositions[pair.second.first] = pair.second.second;
    currentPositions.erase(pair.first);
}

void EnemyTracker::updateFoggyPositions() {
    for (auto& It = foggyPositions.begin(); It != foggyPositions.end();) {
        if (!isInFog(It->first))
            foggyPositions.erase(It++);
        else
            ++It;
    }
}

bool EnemyTracker::isInFog(const BWAPI::Position& position) {
    return !BWAPI::Broodwar->isVisible(BWAPI::TilePosition(position));
}

BWAPI::Position EnemyTracker::getClosestEnemyPosition(
    const BWAPI::Position& sourcePosition) const
{
    if (empty())
        return BWAPI::Positions::None;
    std::vector<BWAPI::Position> loggedPositions;
    for (const std::pair<BWAPI::Unit, PositionalType>& pair: currentPositions)
        loggedPositions.push_back(pair.second.first);
    for (const auto& pair: foggyPositions)
        loggedPositions.push_back(pair.first);
    return (*std::min_element(
        loggedPositions.begin(), loggedPositions.end(),
        Utils::Position(sourcePosition).comparePositions()));
}

EnemyTracker::iterator EnemyTracker::begin() {
    return iterator(
        currentPositions.cbegin(), currentPositions.cend(),
        foggyPositions.cbegin(), foggyPositions.cend());
}

EnemyTracker::iterator EnemyTracker::end() {
    return iterator(
        currentPositions.cend(), currentPositions.cend(),
        foggyPositions.cend(), foggyPositions.cend());
}

EnemyTracker::iterator& EnemyTracker::iterator::operator++() {
    if (cIt == cEnd)
        ++fIt;
    else
        ++cIt;
    return *this;
}

bool EnemyTracker::iterator::operator==(iterator other) const {
    return (cIt == other.cIt && fIt == other.fIt);
}

bool EnemyTracker::iterator::operator!=(iterator other) const {
    return !(*this == other);
}

PositionalType EnemyTracker::iterator::operator*() const {
    if (cIt != cEnd)
        return cIt->second;
    return *fIt;
}
