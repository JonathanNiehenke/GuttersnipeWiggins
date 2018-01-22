#ifndef ZERGRACE_CPP
#define ZERGRACE_CPP
#include "ZergRace.h"
#include <cassert>

void ZergRace::handleEggType(BWAPI::Unit Unit)
{
    BWAPI::UnitType insideEggType = Unit->getBuildType();
    if (insideEggType == supplyType) {
        ++incompleteOverlords;
        assembleSquads();  // Empty squads are Ok.
    }
}

void ZergRace::onUnitCreate(BWAPI::Unit Unit)
{
    // Created zerg units are larva, zergling or from the games's start
    switch (Unit->getType()) {
        // Because we expect it, catch it away from default.
        case BWAPI::UnitTypes::Enum::Zerg_Larva:
        case BWAPI::UnitTypes::Enum::Zerg_Drone:
        case BWAPI::UnitTypes::Enum::Zerg_Zergling:  // Second zergling
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Overlord:
            ++incompleteOverlords;
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Hatchery:
            unitTrainer->includeFacility(Unit);
            break;
        default:
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << " created!" << std::endl;
    }
}

void ZergRace::onUnitMorph(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        // case BWAPI::UnitTypes::Enum::Zerg_Larva:
        case BWAPI::UnitTypes::Enum::Zerg_Drone:
        case BWAPI::UnitTypes::Enum::Zerg_Overlord:
        case BWAPI::UnitTypes::Enum::Zerg_Zergling:
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Egg:
            handleEggType(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Spawning_Pool:
            if (unitTrainer->facilityCount() == 1) {
                scout(cartographer->getStartingLocations());
            }
            buildingConstructor->onCreate(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Hatchery:
            unitTrainer->includeFacility(Unit);
            cartographer->addFacilityPosition(Unit->getPosition());
            buildingConstructor->onCreate(Unit);
            break;
        default: 
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << " morphed!:" << std::endl;
    }
}

void ZergRace::onCenterComplete(BWAPI::Unit Unit)
{
    
    cartographer->addFacilityPosition(Unit->getPosition());
    // Prevent army hatcheries from becoming an EcoBases;
    if (!Unit->getClosestUnit(BWAPI::Filter::IsResourceDepot, 350))
    {
        Race::onCenterComplete(Unit);
    }
}

void ZergRace::onUnitComplete(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        // Because we expect it, catch it away from default.
        case BWAPI::UnitTypes::Enum::Zerg_Larva:
        case BWAPI::UnitTypes::Enum::Zerg_Zergling:
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Drone:
            addWorker(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Overlord:
            --incompleteOverlords;
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Hatchery:
            onCenterComplete(Unit);
            buildingConstructor->onComplete(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Spawning_Pool:
            buildingConstructor->onComplete(Unit);
            break;
        default:
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << " completed!" << std::endl;
    }
}


void ZergRace::onUnitDestroy(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        case BWAPI::UnitTypes::Enum::Zerg_Zergling:
            squadCommander->removeWarrior(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Drone:
            try {
                ecoBaseManager->removeWorker(Unit);
            }
            catch (char* err) {
                BWAPI::Broodwar->sendText(err);
            }
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Overlord:
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Spawning_Pool:
            BWAPI::Broodwar->sendText("gg, you've proven more superior.");
            BWAPI::Broodwar->leaveGame();
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Hatchery:
            ecoBaseManager->removeBase(Unit);  // Even if constructing
            unitTrainer->removeFacility(Unit);
            cartographer->removeFacilityPosition(Unit->getPosition());
            buildingConstructor->onComplete(Unit);
            break;
        default:
            BWAPI::Broodwar << "Unexpected " << Unit->getType().c_str()
                            << " destroyed!" << std::endl;
    }
}

void ZergRace::createFacility()
{
    // Instead of multiple spawning pools build hatcharies.
    if (Self->allUnitCount(armyTechType)) {
        buildingConstructor->request(centerType);
    }
    else {
        buildingConstructor->request(armyTechType);
    }
}

int ZergRace::getAvailableSupply()
{
    const int suppProvided = supplyType.supplyProvided(),
              centProvided = centerType.supplyProvided();
    int constructingCenter = Self->incompleteUnitCount(centerType),
        overlordSupply = incompleteOverlords * suppProvided,
        supplyToBeProvided = (
            overlordSupply + constructingCenter * centProvided);
    return Self->supplyTotal() + supplyToBeProvided - Self->supplyUsed();
}

bool ZergRace::needsSupply()
{
    int workerBuffer = getUnitBuffer(workerType);
    // Zerg facilities do not produce till completed armyTechType
    int armyBuffer = (Self->completedUnitCount(armyTechType)
                      ?  getUnitBuffer(armyUnitType) : 0);
    return getAvailableSupply() <= workerBuffer + armyBuffer;
}

bool ZergRace::readyForTeir1Tech()
{
    const int facilityPrice = armyTechType.mineralPrice();
    int armyBuffer = 50 * Self->completedUnitCount(centerType),
        mineralsToBuild = facilityPrice + armyBuffer;
    return (Self->minerals() > mineralsToBuild || !unitTrainer->isAvailable());
}

void ZergRace::displayStatus()
{
    BWAPI::Broodwar->drawTextScreen(3, 15,
        "availableSupply: %d, Buffer: %d, pendingSupply %d ",
        getAvailableSupply(), 
        getUnitBuffer(workerType) + (Self->completedUnitCount(armyTechType)
                      ?  getUnitBuffer(armyUnitType) : 0),
        incompleteOverlords);
    int row = 30;
    ecoBaseManager->displayStatus(row);
    unitTrainer->displayStatus(row);
    squadCommander->displayStatus(row);
    cartographer->displayStatus(row);
}


#endif

