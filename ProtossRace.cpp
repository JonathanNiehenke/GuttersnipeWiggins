#pragma once
#include "ProtossRace.h"

void ProtossRace::onUnitCreate(const BWAPI::Unit& createdUnit) {
    if (createdUnit->getType() == BWAPI::UnitTypes::Protoss_Gateway)
        armyTrainer->includeFacility(createdUnit);
    Race::onUnitCreate(createdUnit);
}

void ProtossRace::onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding) {
    buildingConstructor->onComplete(destroyedBuilding);
    if (destroyedBuilding->getType() == BWAPI::UnitTypes::Protoss_Gateway)
        armyTrainer->removeFacility(destroyedBuilding);
}

void ProtossRace::construct(const BWAPI::UnitType& buildingType) {
    if (buildingType.requiresPsi() && !doesPylonExist())
        buildingConstructor->request(supplyType);
    else
        buildingConstructor->request(buildingType);
}

bool ProtossRace::doesPylonExist() const {
    return BWAPI::Broodwar->self()->allUnitCount(supplyType) > 0;
}
