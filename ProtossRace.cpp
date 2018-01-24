#pragma once
#include "ProtossRace.h"

void ProtossRace::onUnitCreate(const BWAPI::Unit& createdUnit) {
    if (createdUnit->getType() == BWAPI::UnitTypes::Protoss_Gateway)
        armyTrainer->includeFacility(createdUnit);
    Race::onCreate(createdUnit);
}

void ProtossRace::onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding) {
    buildingConstructor->onComplete(destroyedBuilding);
    if (destroyedBuilding->getType() == BWAPI::UnitTypes::Protoss_Gateway)
        armyTrainer->removeFacility(Unit);
}

void ProtossRace::construct(const BWAPI::UnitType& buildingType) {
    // May request second pylon during the first's production
    if (buildingType.requiresPsi() && Self->completedUnitCount(supplyType))
        buildingConstructor->request(supplyType);
    else
        buildingConstructor->request(buildingType);
}

