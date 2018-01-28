#pragma once
#include "ArmyTrainer.h"

using namespace BWAPI::Filter;

void ArmyTrainer::includeFacility(const BWAPI::Unit& Facility) {
    productionFacilities.push_back(Facility);
}

void ArmyTrainer::removeFacility(const BWAPI::Unit& Facility) {
    auto endIt = productionFacilities.end(),
         foundIt = find(productionFacilities.begin(), endIt, Facility);
    if (foundIt != endIt)
        productionFacilities.erase(foundIt);
}

int ArmyTrainer::facilityCount() const {
    return productionFacilities.size();
}

bool ArmyTrainer::readyToTrain() const {
    for (const BWAPI::Unit& Facility: productionFacilities) {
        if (isTrainingQueueEmpty(Facility))
            return true;
    }
    return false;
}

void ArmyTrainer::trainUnits(const BWAPI::UnitType& unitType) {
    for (BWAPI::Unit& Facility: productionFacilities) {
        if (isTrainingQueueEmpty(Facility))
            Facility->train(unitType);
    }
}

bool ArmyTrainer::isTrainingQueueEmpty(const BWAPI::Unit& Facility) const {
    // Because getLarva() returns Unitset and getTrainingQueue() deque
    if (Facility->getType().producesLarva())
        return !Facility->getLarva().empty();
    else
        return Facility->getTrainingQueue().empty();
}
