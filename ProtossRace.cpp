#pragma once
#include "ProtossRace.h"

void ProtossRace::onUnitCreate(const BWAPI::Unit& createdUnit) const {
    if (createdUnit->getType() == BWAPI::UnitTypes::Protoss_Gateway)
        armyTrainer->includeFacility(createdUnit);
    Race::onUnitCreate(createdUnit);
}

void ProtossRace::onDestroyedBuilding(
    const BWAPI::Unit& destroyedBuilding) const
{
    buildingConstructor->onComplete(destroyedBuilding);
    if (destroyedBuilding->getType() == BWAPI::UnitTypes::Protoss_Gateway)
        armyTrainer->removeFacility(destroyedBuilding);
}

void ProtossRace::construct(const BWAPI::UnitType& buildingType) const {
        buildingConstructor->request(buildingType);
}

BWAPI::UnitType ProtossRace::getNextRequiredBuilding(
    const BWAPI::UnitType& unitType) const
{
    const auto& requiredType = Race::getNextRequiredBuilding(unitType);
    return (requiredType.requiresPsi() && !doesPylonExist()
        ? BWAPI::UnitTypes::Protoss_Pylon : requiredType);
}

bool ProtossRace::doesPylonExist() const {
    return BWAPI::Broodwar->self()->allUnitCount(supplyType) > 0;
}
