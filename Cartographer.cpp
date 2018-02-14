#pragma once
#include "Cartographer.h"

using namespace BWAPI::Filter;

void Cartographer::discoverResourcePositions() {
    for (const auto& pair: getStarcraftMappedResources()) {
        const BWAPI::Unitset& groupedResources = pair.second;
        resourcePositions.push_back(groupedResources.getPosition());
    }
}

std::map<int, BWAPI::Unitset> Cartographer::getStarcraftMappedResources() {
    std::map<int, BWAPI::Unitset> groupsOfResources;
    groupResources(BWAPI::Broodwar->getStaticMinerals(), groupsOfResources);
    groupResources(BWAPI::Broodwar->getStaticGeysers(), groupsOfResources);
    return groupsOfResources;
}

void Cartographer::groupResources(
    const BWAPI::Unitset& Resources,
    std::map<int, BWAPI::Unitset>& groupedResources)
{
    for (BWAPI::Unit Resource: Resources)
        groupedResources[Resource->getResourceGroup()].insert(Resource);
}

std::vector<BWAPI::Position> Cartographer::getUnexploredStartingPositions() {
    std::vector<BWAPI::Position> startingPositions;
    for (BWAPI::TilePosition Start:BWAPI::Broodwar->getStartLocations()) {
        if (!BWAPI::Broodwar->isExplored(Start))
            startingPositions.push_back(BWAPI::Position(Start));
    }
    return startingPositions;
}

std::vector<BWAPI::Position> Cartographer::getStartingPositions() {
    std::vector<BWAPI::Position> startingPositions;
    // Prevents scouting for our and allies bases.
    for (BWAPI::TilePosition Start:BWAPI::Broodwar->getStartLocations()) {
        if (!BWAPI::Broodwar->isVisible(Start))
            startingPositions.push_back(BWAPI::Position(Start));
    }
    return startingPositions;
}

bool Cartographer::lacksEnemySighting() const {
    return evasionTracker.empty();
}

void Cartographer::addUnit(const BWAPI::Unit& unit) {
    evasionTracker.addUnit(unit);
}

void Cartographer::removeUnit(const BWAPI::Unit& unit) {
    evasionTracker.removeUnit(unit);
}

void Cartographer::update() {
    evasionTracker.update();
}

BWAPI::Position Cartographer::getNextPosition(
    const BWAPI::Position& sourcePosition)
{
    if (!evasionTracker.empty())
        return evasionTracker.getClosestEnemyPosition(sourcePosition);
    const auto& startPositions = getUnexploredStartingPositions();
    if (!startPositions.empty())
        return startPositions[attackIdx++ % startPositions.size()];
    return BWAPI::Positions::None;
}


BWAPI::Position Cartographer::getClosestEnemyPosition(
    const BWAPI::Position& sourcePosition) const
{
    return evasionTracker.getClosestEnemyPosition(sourcePosition);
}

void Cartographer::drawStatus() {
    for (const PositionalType positionalType: evasionTracker) {
        BWAPI::Broodwar->drawTextMap(
            positionalType.first, positionalType.second.c_str());
    }
}

bool EvasionTracker::empty() const {
    return currentPositions.empty() && foggyPositions.empty();
}

void EvasionTracker::addUnit(const BWAPI::Unit& unit) {
    currentPositions[unit] = PositionalType(
        unit->getPosition(), unit->getType());
}

void EvasionTracker::removeUnit(const BWAPI::Unit& unit) {
    currentPositions.erase(unit);
}

void EvasionTracker::update() {
    updateCurrentPositions();
    updateFoggyPositions();
}

void EvasionTracker::updateCurrentPositions() {
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

void EvasionTracker::moveToFoggyPositions(
    const std::pair<BWAPI::Unit, PositionalType>& pair)
{
    foggyPositions[pair.second.first] = pair.second.second;
    currentPositions.erase(pair.first);
}

void EvasionTracker::updateFoggyPositions() {
    for (auto& It = foggyPositions.begin(); It != foggyPositions.end();) {
        if (!isInFog(It->first))
            foggyPositions.erase(It++);
        else
            ++It;
    }
}

bool EvasionTracker::isInFog(const BWAPI::Position& position) {
    return !BWAPI::Broodwar->isVisible(BWAPI::TilePosition(position));
}

BWAPI::Position EvasionTracker::getClosestEnemyPosition(
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

EvasionTracker::iterator EvasionTracker::begin() {
    return iterator(
        currentPositions.cbegin(), currentPositions.cend(),
        foggyPositions.cbegin(), foggyPositions.cend());
}

EvasionTracker::iterator EvasionTracker::end() {
    return iterator(
        currentPositions.cend(), currentPositions.cend(),
        foggyPositions.cend(), foggyPositions.cend());
}

EvasionTracker::iterator& EvasionTracker::iterator::operator++() {
    if (cIt == cEnd)
        ++fIt;
    else
        ++cIt;
    return *this;
}

bool EvasionTracker::iterator::operator==(iterator other) const {
    return (cIt == other.cIt && fIt == other.fIt);
}

bool EvasionTracker::iterator::operator!=(iterator other) const {
    return !(*this == other);
}

PositionalType EvasionTracker::iterator::operator*() const {
    if (cIt != cEnd)
        return cIt->second;
    return *fIt;
}
