#ifndef UTILS_CPP
#define UTILS_CPP
#include "Utils.h"

bool Utils::compareDistanceFrom::operator()(
        BWAPI::Position Pos1, BWAPI::Position Pos2)
{
    return (sourcePosition.getApproxDistance(Pos1) <
            sourcePosition.getApproxDistance(Pos2));
}

bool Utils::compareDistanceFrom::operator()(
        BWAPI::TilePosition tPos1, BWAPI::TilePosition tPos2)
{
    return (sourcePosition.getApproxDistance(BWAPI::Position(tPos1)) <
            sourcePosition.getApproxDistance(BWAPI::Position(tPos2)));
}

bool Utils::compareDistanceFrom::operator()(BWAPI::Unit u1, BWAPI::Unit u2)
{
    return (sourcePosition.getApproxDistance(u1->getPosition()) <
            sourcePosition.getApproxDistance(u2->getPosition()));
}

bool Utils::compareDistanceFrom::operator()(
        BWAPI::Unitset u1, BWAPI::Unitset u2)
{
    return (sourcePosition.getApproxDistance(u1.getPosition()) <
            sourcePosition.getApproxDistance(u2.getPosition()));
}

bool Utils::isIdle(BWAPI::Unit Facility)
{
    // Zerg hatchery is always idle so determine with larva.
    // Negate larva for Protoss and Terran with producesLarva.
    return (Facility->isIdle() && !(Facility->getType().producesLarva() &&
            Facility->getLarva().empty()));
}

#endif
