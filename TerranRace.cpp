#pragma once
#include "TerranRace.h"

void TerranRace::onUnitCreate(const BWAPI::Unit& createdUnit) const {
    if (createdUnit->getType() == BWAPI::UnitTypes::Terran_Barracks)
        armyTrainer->includeFacility(createdUnit);
    Race::onUnitCreate(createdUnit);
}

void TerranRace::onDestroyedBuilding(const BWAPI::Unit& destroyedBuilding) const {
    buildingConstructor->onComplete(destroyedBuilding);
    if (destroyedBuilding->getType() == BWAPI::UnitTypes::Terran_Barracks)
        armyTrainer->removeFacility(destroyedBuilding);
}
