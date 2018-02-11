#pragma once
#include "ZergRace.h"

void ZergRace::onUnitMorph(const BWAPI::Unit& morphedUnit) {
    if (isIncompleteOverlord(morphedUnit))
        ++incompleteOverlordCount;
    onUnitCreate(morphedUnit);
}

bool ZergRace::isIncompleteOverlord(const BWAPI::Unit& unit) {
    return (unit->getType() == BWAPI::UnitTypes::Zerg_Egg &&
        unit->getBuildType() == BWAPI::UnitTypes::Zerg_Overlord);
}

void ZergRace::onUnitComplete(const BWAPI::Unit& completedUnit) {
    if (completedUnit->getType() == supplyType && incompleteOverlordCount > 0)
        --incompleteOverlordCount;
    Race::onUnitComplete(completedUnit);
}

int ZergRace::expectedSupplyProvided(
    const BWAPI::UnitType& providerType) const
{
    int providerCount = BWAPI::Broodwar->self()->allUnitCount(
        providerType);
    if (providerType == supplyType)
       providerCount  += incompleteOverlordCount;
    return providerType.supplyProvided() * providerCount;
}

void ZergRace::createSupply() const {
    unitTrainer->trainUnits(supplyType);
}

void ZergRace::construct(const BWAPI::UnitType& buildingType) const {
    if (buildingType == centerType || doesTechExist(buildingType))
        buildingConstructor->request(centerType);
    else
        buildingConstructor->request(buildingType);
}

bool ZergRace::doesTechExist(const BWAPI::UnitType& buildingType) const {
    return BWAPI::Broodwar->self()->allUnitCount(buildingType) > 0;
}
