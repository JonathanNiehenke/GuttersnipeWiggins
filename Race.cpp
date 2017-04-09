#ifndef RACE_CPP
#define RACE_CPP
#include "Race.h"

// To reduce the parameters list of Race and derived constructors.
Core::Core(
    BuildingConstructer *buildingConstructer,
    Cartographer *cartographer,
    CmdRescuer::Rescuer *cmdRescuer,
    EcoBaseManager *ecoBaseManager,
    SquadCommander *squadCommander,
    UnitTrainer *unitTrainer)
{
    this->buildingConstructer = buildingConstructer;
    this->cartographer = cartographer;
    this->cmdRescuer = cmdRescuer;
    this->ecoBaseManager = ecoBaseManager;
    this->squadCommander = squadCommander;
    this->unitTrainer = unitTrainer;
}


Race::Race(
    BWAPI::UnitType centerType,
    BWAPI::UnitType workerType,
    BWAPI::UnitType supplyType,
    BWAPI::UnitType armyTechType,
    BWAPI::UnitType armyUnitType,
    Core &core)
{
    this->centerType = centerType;
    this->workerType = workerType;
    this->supplyType = supplyType;
    this->armyTechType = armyTechType;
    this->armyUnitType = armyUnitType;
    this->buildingConstructer = core.buildingConstructer;
    this->cartographer = core.cartographer;
    this->cmdRescuer = core.cmdRescuer;
    this->ecoBaseManager = core.ecoBaseManager;
    this->squadCommander = core.squadCommander;
    this->unitTrainer = core.unitTrainer;
    this->Self = BWAPI::Broodwar->self();
}

void Race::onCenterComplete(BWAPI::Unit Unit)
{
    // ToDo: Rewrite.
    for (BWAPI::Unitset mineralCluster: cartographer->getMinerals()) {
        BWAPI::Unit baseCenter = BWAPI::Broodwar->getClosestUnit(
            mineralCluster.getPosition(), BWAPI::Filter::IsResourceDepot, 300);
        if (baseCenter == Unit) {
            ecoBaseManager->addBase(baseCenter, mineralCluster);
            break;
        }
    }
}

int Race::getAvailableSupply()
{
    const int suppProvided = supplyType.supplyProvided(),
              centProvided = centerType.supplyProvided();
    int creatingSupply = Self->incompleteUnitCount(supplyType),
        constructingCenter = Self->incompleteUnitCount(centerType),
        supplyToBeProvided = (
            creatingSupply * suppProvided + constructingCenter * centProvided);
    return Self->supplyTotal() + supplyToBeProvided - Self->supplyUsed();
}

int Race::getUnitBuffer(BWAPI::UnitType unitType)
{
    // Prevents repetitive conversion into a float.
    const float supplyBuildTime = supplyType.buildTime();
    int unitsPerSupplyBuild = int(std::ceil(
            supplyBuildTime / unitType.buildTime())),
        facilityAmount = (unitType == workerType
            ? ecoBaseManager->getBaseAmount()
            : unitTrainer->facilityCount()),
        unitSupplyUsed = unitType.supplyRequired();
    return unitsPerSupplyBuild * facilityAmount * unitSupplyUsed;
}

bool Race::needsSupply()
{
    int workerBuffer = getUnitBuffer(workerType),
        armyBuffer = getUnitBuffer(armyUnitType);
    return getAvailableSupply() <= workerBuffer + armyBuffer;
}

bool Race::readyToExpand()
{
    return !Self->incompleteUnitCount(centerType) &&
        unitTrainer->facilityCount() >= 2 && ecoBaseManager->isAtCapacity();
}

bool Race::readyForArmyTech()
{
    const int facilityPrice = armyTechType.mineralPrice();
    return (Self->minerals() > facilityPrice * 1.5 ||
            !unitTrainer->isAvailable());
}

void Race::manageProduction()
{
    // ToDo: Max supply condition relative to game mode (Melee).
    if (needsSupply() && Self->supplyTotal() != 400)
    {
        createSupply();
    }
    else {
        try {
            createWorkers();
        }
        catch (char* err) {
            BWAPI::Broodwar->sendText(err);
        }
        createWarriors();
    }
}

void Race::manageStructures()
{
    if (readyToExpand()) {
        createCenter();
    }
    // Reposition: uses supplyType.
    else if (readyForArmyTech()) {
        createFacility();
    }
}

void Race::manageAttackGroups()
{
    // ToDo: Change defense to around given positions.
    for (BWAPI::Position Pos: cartographer->getFacilityPositions()) {
        if (BWAPI::Broodwar->getClosestUnit(Pos, BWAPI::Filter::IsEnemy, 100))
        {
            squadCommander->assembleSquad();  
            break;  // Enemy is inside base assemble defenders.
        }
    }
    squadCommander->uniteSquads();
    squadCommander->removeEmptySquads();
}

// Workaround cause initial workers may complete before the centerType.
// To associate a worker to an EcoBase it must first exist.
void Race::onCompleteWorkaround(BWAPI::Unit Unit)
{
    // C3480 requires the capture variable to originate within scope
    EcoBaseManager *lambdaTemp = ecoBaseManager;
    BWAPI::Broodwar->registerEvent(
        [Unit, lambdaTemp](BWAPI::Game*) { lambdaTemp->addWorker(Unit); },
        nullptr, 1);
}

void Race::scout(std::set<BWAPI::TilePosition> scoutLocations)
{
    using namespace BWAPI::Filter;
    // ToDo: Scout with fewer workers.
    // Bug: Protoss scout consistency.
    BWAPI::Unit baseCenter = ecoBaseManager->getCenter();
    BWAPI::Unitset workerUnits = baseCenter->getUnitsInRadius(
        900, IsWorker && IsOwned && !IsConstructing);
    auto workerIt = workerUnits.begin();
    for (BWAPI::TilePosition Location: scoutLocations) {
        // ToDo: Gather the previous mineral instead.
        BWAPI::Unit Scout = *workerIt++,
                    closestMineral = Scout->getClosestUnit(IsMineralField);
        if (!Scout->move(BWAPI::Position(Location))) {
            cmdRescuer->append(CmdRescuer::MoveCommand(
                Scout, BWAPI::Position(Location), true));
            cmdRescuer->append(CmdRescuer::GatherCommand(
                Scout, closestMineral, true));
        }
        else if (!Scout->gather(closestMineral, true)) {
            cmdRescuer->append(CmdRescuer::GatherCommand(
                Scout, closestMineral, true));
        }
    }
}

void Race::displayStatus()
{
    BWAPI::Broodwar->drawTextScreen(3, 15,
        "availableSupply: %d, Buffer: %d, pendingSupply %d ",
        getAvailableSupply(), 
        getUnitBuffer(workerType) + getUnitBuffer(armyUnitType),
        Self->incompleteUnitCount(supplyType));
    int row = 30;
    ecoBaseManager->displayStatus(row);
    unitTrainer->displayStatus(row);
    buildingConstructer->displayStatus(row);
    squadCommander->displayStatus(row);
    cartographer->displayStatus(row);
}

#endif