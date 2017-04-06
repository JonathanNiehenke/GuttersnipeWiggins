#ifndef GUTTERSNIPEWIGGINS_CPP
#define GUTTERSNIPEWIGGINS_CPP
#include "GuttersnipeWiggins.h"

// ToDo: UnitTrainer.

using namespace BWAPI::Filter;

typedef std::pair<BWAPI::Position, BWAPI::Unitset> PositionedUnits;
typedef std::set<BWAPI::TilePosition> locationSet;

void GW::assignFields()
{
    Self = BWAPI::Broodwar->self();
    BWAPI::Race myRace = Self->getRace();
    centerType = myRace.getCenter();
    supplyType = myRace.getSupplyProvider();
    workerType = myRace.getWorker();
    baseCenter = BWAPI::Broodwar->getClosestUnit(
        BWAPI::Position(Self->getStartLocation()), IsResourceDepot);
    // Used only by Zerg. Zerg no longer works.
    // TRAINING[supplyType].includeFacility(baseCenter);
    switch (myRace) {
        case BWAPI::Races::Enum::Protoss: // Enum for constant value.
            armyEnablingTechType = BWAPI::UnitTypes::Protoss_Gateway;
            armyUnitType = BWAPI::UnitTypes::Protoss_Zealot;
            break;
        case BWAPI::Races::Enum::Terran:
            armyEnablingTechType = BWAPI::UnitTypes::Terran_Barracks;
            armyUnitType = BWAPI::UnitTypes::Terran_Marine;
            break;
        case BWAPI::Races::Enum::Zerg:
            armyEnablingTechType = BWAPI::UnitTypes::Zerg_Spawning_Pool;
            armyUnitType = BWAPI::UnitTypes::Zerg_Zergling;
            break;
    }
}

void GW::onStart()
{
    BWAPI::Broodwar->enableFlag(1);
    assignFields();
    cartographer.discoverResources(BWAPI::Position(Self->getStartLocation()));
    // Workaround cause initial workers may complete before the baseCenter.
    for (BWAPI::Unitset mineralCluster: cartographer.getMinerals()) {
        BWAPI::Unit baseCenter = BWAPI::Broodwar->getClosestUnit(
            mineralCluster.getPosition(), IsResourceDepot, 300);
        if (baseCenter == baseCenter) {
            ecoBaseManager.addBase(baseCenter, mineralCluster);
            break;
        }
    }
    workerBuffer = GW::getUnitBuffer(workerType);
    squadCommander.onStart(baseCenter, armyUnitType, Self, &cartographer);
    buildingConstructer.onStart(Self, baseCenter, &cmdRescuer, &cartographer);
    warriorTrainer.onStart(armyUnitType, &cmdRescuer);
}

void GW::onFrame()
{
    const int actionFrames = std::max(5, BWAPI::Broodwar->getLatency());
    GW::displayStatus(); // For debugging.
    switch(BWAPI::Broodwar->getFrameCount() % actionFrames) {
        case 0: GW::manageProduction();
            break;
        case 1: GW::manageBases();
            break;
        case 2: GW::manageAttackGroups();
            break;
        case 3: squadCommander.combatMicro();
            break;
        case 4:
            cmdRescuer.rescue();
            cartographer.cleanEnemyLocations();
            if (Self->supplyUsed() == 400) {
                squadCommander.assembleSquad();
            }
            break;
        default: break;
    }
}

void GW::onBuildingCreate(BWAPI::Unit Unit)
{
    buildingConstructer.addProduct(Unit);
    switch (Unit->getType()) {
        case BWAPI::UnitTypes::Enum::Protoss_Pylon:
        case BWAPI::UnitTypes::Enum::Terran_Supply_Depot:
            squadCommander.assembleSquad();
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Gateway:
        case BWAPI::UnitTypes::Enum::Terran_Barracks:
            if (!warriorTrainer.isAvailable()) {
                scout(cartographer.getStartingLocations());
            }
            warriorTrainer.includeFacility(Unit);
            armyBuffer = getUnitBuffer(armyUnitType);
        // Because we expect it catch it away from default.
        case BWAPI::UnitTypes::Enum::Protoss_Nexus:
        case BWAPI::UnitTypes::Enum::Terran_Command_Center:
            break;
        default: 
            BWAPI::Broodwar << "Unexpected Building created: "
                            << Unit->getType().c_str() << std::endl;
    }
}

void GW::onUnitCreate(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != Self) return;  // Ignoring non-owned units
    BWAPI::UnitType unitType = Unit->getType();
    if (unitType.isBuilding()) {
        onBuildingCreate(Unit);
    }
    PENDING_UNIT_TYPE_COUNT[Unit->getType()]++;
    // Always after change to pending count.
    availableSupply = GW::getAvailableSupply();
}

void GW::handleEggType(BWAPI::Unit Unit)
{
    BWAPI::UnitType insideEggType = Unit->getBuildType();
    if (insideEggType == supplyType && warriorTrainer.isAvailable()) {
        squadCommander.assembleSquad();
    }
    PENDING_UNIT_TYPE_COUNT[insideEggType]++;
    // Always after change to pending count.
    availableSupply = GW::getAvailableSupply();
}

void GW::onBuildingMorph(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        case BWAPI::UnitTypes::Enum::Zerg_Hatchery:
            warriorTrainer.includeFacility(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Spawning_Pool:
            warriorTrainer.includeFacility(baseCenter);
            scout(cartographer.getStartingLocations());
            break;
        default: 
                BWAPI::Broodwar << "Unexpected Building created: "
                                << Unit->getType().c_str() << std::endl;
    }
    buildingConstructer.addProduct(Unit);
    PENDING_UNIT_TYPE_COUNT[Unit->getType()]++;
    // Zerg workers become buildings, so recalculate.
    availableSupply = GW::getAvailableSupply();
    armyBuffer = getUnitBuffer(armyUnitType);
}

void GW::onUnitMorph(BWAPI::Unit Unit)
{
    BWAPI::UnitType unitType = Unit->getType();
    if (Unit->getPlayer() != Self) {
        // Because geyser structures are never destroyed.
        if (unitType == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
            cartographer.removeBuildingLocation(Unit->getTilePosition());
        }
        return;  // Ignoring the other non-owned units.
    }
    if (unitType == BWAPI::UnitTypes::Zerg_Egg) {
        handleEggType(Unit);
    }
    else if (unitType.isBuilding()) {
        onBuildingMorph(Unit);
    }
}

void GW::onCenterComplete(BWAPI::Unit Unit)
{
    if (!Unit->getClosestUnit(IsResourceDepot, 350) && Unit != baseCenter) {
        workerBuffer = GW::getUnitBuffer(workerType);
        BWAPI::Broodwar << "Searching for minerals" << std::endl;
        // Add to ecoBaseManager after finding nearest mineralCluster.
        for (BWAPI::Unitset mineralCluster: cartographer.getMinerals()) {
            BWAPI::Unit baseCenter = BWAPI::Broodwar->getClosestUnit(
                mineralCluster.getPosition(), IsResourceDepot, 300);
            if (baseCenter == Unit) {
                BWAPI::Broodwar << "Creating EcoBase" << std::endl;
                ecoBaseManager.addBase(baseCenter, mineralCluster);
                break;
            }
        }
    }
}

void GW::onUnitComplete(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() != Self) return;  // Ignoring non-owned units
    PENDING_UNIT_TYPE_COUNT[Unit->getType()]--;
    // Always after change to pending count.
    availableSupply = GW::getAvailableSupply();
    switch (Unit->getType()) {
        // Because we expect it, catch it away from default.
        case BWAPI::UnitTypes::Enum::Protoss_Zealot:
        case BWAPI::UnitTypes::Enum::Terran_Marine:
        case BWAPI::UnitTypes::Enum::Zerg_Zergling:
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Probe:
        case BWAPI::UnitTypes::Enum::Terran_SCV:
        case BWAPI::UnitTypes::Enum::Zerg_Drone:
            try {
                ecoBaseManager.addWorker(Unit);
            }
            catch (char* err) {
                BWAPI::Broodwar->sendText(err);
            }
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Pylon:
        case BWAPI::UnitTypes::Enum::Terran_Supply_Depot:
        case BWAPI::UnitTypes::Enum::Zerg_Overlord:
            ++supplyCount;
            buildingConstructer.removeConstruction(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Nexus:
        case BWAPI::UnitTypes::Enum::Terran_Command_Center:
        case BWAPI::UnitTypes::Enum::Zerg_Hatchery:
            onCenterComplete(Unit);
            buildingConstructer.removeConstruction(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Gateway:
        case BWAPI::UnitTypes::Enum::Terran_Barracks:
            buildingConstructer.removeConstruction(Unit);
            break;
        default:
            BWAPI::Broodwar << "Unexpected unit completed: "
                            << Unit->getType().c_str() << std::endl;
    }
}

void GW::onUnitLoss(BWAPI::Unit Unit)
{
    switch (Unit->getType()) {
        case BWAPI::UnitTypes::Enum::Protoss_Zealot:
        case BWAPI::UnitTypes::Enum::Terran_Marine:
        case BWAPI::UnitTypes::Enum::Zerg_Zergling:
            squadCommander.removeWarrior(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Probe:
        case BWAPI::UnitTypes::Enum::Terran_SCV:
        case BWAPI::UnitTypes::Enum::Zerg_Drone:
            try {
                ecoBaseManager.removeWorker(Unit);
            }
            catch (char* err) {
                BWAPI::Broodwar->sendText(err);
            }
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Pylon:
        case BWAPI::UnitTypes::Enum::Terran_Supply_Depot:
        case BWAPI::UnitTypes::Enum::Zerg_Overlord:
            if (Unit->isCompleted()) {
                --supplyCount;
            }
            buildingConstructer.removeConstruction(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Gateway:
        case BWAPI::UnitTypes::Enum::Terran_Barracks:
            warriorTrainer.removeFacility(Unit);
            buildingConstructer.removeConstruction(Unit);
            break;
        case BWAPI::UnitTypes::Enum::Zerg_Spawning_Pool:
            BWAPI::Broodwar->sendText("gg, you've proven more superior.");
            BWAPI::Broodwar->leaveGame();
            break;
        case BWAPI::UnitTypes::Enum::Protoss_Nexus:
        case BWAPI::UnitTypes::Enum::Terran_Command_Center:
        case BWAPI::UnitTypes::Enum::Zerg_Hatchery:
            ecoBaseManager.removeBase(Unit);
            buildingConstructer.removeConstruction(Unit);
            break;
        default:
            BWAPI::Broodwar << "Unexpected unit destroyed: "
                            << Unit->getType().c_str() << std::endl;
    }
}

void GW::onUnitDestroy(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() == Self) {
        onUnitLoss(Unit);
    }
    else if (Unit->getType().isMineralField()) {
        try {
            ecoBaseManager.removeMineral(Unit);
        }
        catch (char* err) {
            BWAPI::Broodwar->sendText(err);
        }
    }
    else if (Unit->getType().isBuilding()) {
        cartographer.removeBuildingLocation(
            Unit->getPlayer(), Unit->getTilePosition());
    }
}

void GW::onUnitDiscover(BWAPI::Unit Unit)
{
    BWAPI::Player owningPlayer = Unit->getPlayer();
    if (Self->isEnemy(owningPlayer) && Unit->getType().isBuilding() &&
        !Unit->isFlying())
    {
        cartographer.addBuildingLocation(
            owningPlayer, Unit->getTilePosition());
        // BWAPI::Broodwar->sendTextEx(true, "Enemy %s discovered.",
            // Unit->getType().c_str());
    }
}

void GW::onUnitEvade(BWAPI::Unit Unit)
{
}

void GW::onUnitShow(BWAPI::Unit Unit)
{
}

void GW::onUnitHide(BWAPI::Unit Unit)
{
}

void GW::onUnitRenegade(BWAPI::Unit Unit)
{
    // Perhaps I will learn something.
    BWAPI::Broodwar->sendTextEx(true, "%s is Renegade: %s.",
        Unit->getPlayer()->getName().c_str(), Unit->getType().c_str());
    if (Unit->getPlayer() != Self) return;  // Ignoring non-owned units
}

void GW::onNukeDetect(BWAPI::Position target)
{
}

void GW::onSendText(std::string text)
{
    if (text.substr(0, 7) == "getUnit") {
        int unitId = atoi(text.substr(8, 4).c_str());
        BWAPI::Unit unit = BWAPI::Broodwar->getUnit(unitId);
        if (unit) {
            BWAPI::Broodwar << "Found Unit: %d " << unitId << std::endl;
            BWAPI::Broodwar->setScreenPosition(unit->getPosition());
        }
        return;
    }
    BWAPI::Unitset selectedUnits = BWAPI::Broodwar->getSelectedUnits();
    if (text == "isStuck") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Broodwar->sendTextEx(true, "%d: %s",
                unit->getID(), unit->isStuck() ? "True" : "False");
            }
    }
    else if (text == "getPosition") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Position Pos = unit->getPosition();
            BWAPI::Broodwar->sendTextEx(true, "%d: (%d, %d)",
                unit->getID(), Pos.x, Pos.y);
        }
    }
    else if (text == "getLocation") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::TilePosition Pos = unit->getTilePosition();
            BWAPI::Broodwar->sendTextEx(true, "%d: (%d, %d)",
                unit->getID(), Pos.x, Pos.y);
        }
    }
    else if (text == "getTargetPosition") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Position TP = unit->getTargetPosition();
            BWAPI::Broodwar->sendTextEx(true, "%d: (%d, %d)",
                unit->getID(), TP.x, TP.y);
        }
    }
    else if (text == "canAttack") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Position TP = unit->getTargetPosition();
            BWAPI::Broodwar->sendTextEx(true,
                unit->getType().groundWeapon() != BWAPI::WeaponTypes::None
                    ? "true" : "false");
        }
    }
    else {
        BWAPI::Broodwar->sendTextEx(true, "'%s'", text.c_str());
    }
}

void GW::onReceiveText(BWAPI::Player player, std::string text)
{
}

void GW::onSaveGame(std::string gameName)
{
}

void GW::onPlayerLeft(BWAPI::Player Player)
{
    cartographer.removePlayerLocations(Player);
}

void GW::onEnd(bool IsWinner)
{
}

void GW::manageProduction()
{
    if (availableSupply <= workerBuffer + armyBuffer &&
        Self->supplyTotal() != 400)
    {
        if (supplyType.isBuilding()) {
            // Constructs a pylon or supply depot.
            buildingConstructer.constructUnit(supplyType);
        }
        else {
            // Trains a overloard.
            // TRAINING[supplyType].produceSingleUnit(supplyType);
        }
    }
    else {
        try {
            ecoBaseManager.produceUnits(workerType);
        }
        catch (char* err) {
            BWAPI::Broodwar->sendText(err);
        }
        warriorTrainer.produceUnits(armyUnitType);
    }
}

void GW::manageBases()
{
    const int centerPrice = centerType.mineralPrice(),
              armyFacilityPrice = armyEnablingTechType.mineralPrice();
    if (!PENDING_UNIT_TYPE_COUNT[centerType] &&
        warriorTrainer.facilityCount() >= 2 &&
        ecoBaseManager.isAtCapacity())
    {
        buildingConstructer.constructExpansion(centerType);
    }
    else if (supplyCount && (Self->minerals() > armyFacilityPrice * 1.5 ||
            !warriorTrainer.isAvailable()))
    {
        // Construct multiple Gateways and Barracks.
        if (armyEnablingTechType.canProduce())
        {
            buildingConstructer.constructUnit(armyEnablingTechType);
        }
        // Instead of multiple spawning pools build hatcharies.
        else if (warriorTrainer.isAvailable() ||
                PENDING_UNIT_TYPE_COUNT[armyEnablingTechType])
        {
            buildingConstructer.constructUnit(BWAPI::UnitTypes::Zerg_Hatchery);
        }
        // This is where we build the spawning pool.
        else
        {
            buildingConstructer.constructUnit(armyEnablingTechType);
        }
    }
}

void GW::manageAttackGroups()
{
    // ToDo: Change defense to around given positons.
    if (baseCenter->getClosestUnit(IsEnemy, 900))
        squadCommander.assembleSquad();  // Enemy is inside base assemble defenders.
    squadCommander.uniteSquads();
    squadCommander.removeEmptySquads();
}

int GW::getAvailableSupply()
{
    const int supply = supplyType.supplyProvided(),
              centerSupply = centerType.supplyProvided();
    int supplyConstructing = (PENDING_UNIT_TYPE_COUNT[supplyType] * supply +
        PENDING_UNIT_TYPE_COUNT[centerType] * centerSupply);
    return Self->supplyTotal() + supplyConstructing - Self->supplyUsed();
}

int GW::getUnitBuffer(BWAPI::UnitType unitType)
{
    // Preventing repetitive conversion into a float by using a float.
    const float supplyBuildTime = supplyType.buildTime();
    int unitSupply = unitType.supplyRequired(),
        unitBuildTime = unitType.buildTime(),
        facilityAmount = (unitType == workerType
            ? ecoBaseManager.getBaseAmount()
            : warriorTrainer.facilityCount()),
        unitsDuringBuild = facilityAmount * std::ceil(
            supplyBuildTime / unitBuildTime);
    return unitsDuringBuild * unitSupply;
}

void GW::scout(std::set<BWAPI::TilePosition> scoutLocations)
{
    // ToDo: Scout with fewer workers.
    // Bug: Won't scout usually Protoss.
    BWAPI::Unitset workerUnits = baseCenter->getUnitsInRadius(
        900, IsWorker && IsOwned && !IsConstructing);
    auto workerIt = workerUnits.begin();
    for (BWAPI::TilePosition Location: scoutLocations) {
        // ToDo: Gather the previous mineral instead.
        BWAPI::Unit Scout = *workerIt++,
                    closestMineral = Scout->getClosestUnit(IsMineralField);
        if (!Scout->move(BWAPI::Position(Location))) {
            cmdRescuer.append(CmdRescuer::MoveCommand(
                Scout, BWAPI::Position(Location), true));
        }
        else if (!Scout->gather(closestMineral, true)) {
            cmdRescuer.append(CmdRescuer::GatherCommand(
                Scout, closestMineral, true));
        }
    }
}

void GW::displayUnitInfo()
{
    int row = 15;
    for (BWAPI::Unit unit: BWAPI::Broodwar->getSelectedUnits()) {
        int sinceCommandFrame = (BWAPI::Broodwar->getFrameCount() -
            unit->getLastCommandFrame());
        BWAPI::UnitCommand lastCmd = unit->getLastCommand();
        BWAPI::Broodwar->drawTextScreen(440, row,
            "%s: %d - %s", unit->getType().c_str(), unit->getID(),
            unit->getOrder().c_str());
        BWAPI::Broodwar->drawTextScreen(440, row + 10,
            "    %d - %s", sinceCommandFrame,
            lastCmd.getType().c_str());
        BWAPI::Unit targetedUnit = unit->getTarget();
        if (targetedUnit) {
            BWAPI::Broodwar->drawLineMap(unit->getPosition(),
                targetedUnit->getPosition(), BWAPI::Color(0, 255, 0));
        }
        if (lastCmd.getType() == BWAPI::UnitCommandTypes::Attack_Unit) {
            BWAPI::Broodwar->registerEvent(
                [unit, lastCmd](BWAPI::Game*)
                    {
                        BWAPI::Broodwar->drawLineMap(unit->getPosition(),
                            lastCmd.getTarget()->getPosition(),
                            BWAPI::Color(255, 0, 0));
                    },
                nullptr, 1);
        }
        row += 25;
    }
}

void GW::displayStatus()
{
    // May perhaps reveal logic errors and bugs.
    BWAPI::Broodwar->drawTextScreen(3, 3, "APM %d, FPS %d, avgFPS %f",
        BWAPI::Broodwar->getAPM(), BWAPI::Broodwar->getFPS(),
        BWAPI::Broodwar->getAverageFPS());
    BWAPI::Broodwar->drawTextScreen(3, 15,
        "availableSupply: %d, Buffer: %d, pendingSupply %d ", availableSupply,
        workerBuffer + armyBuffer, PENDING_UNIT_TYPE_COUNT[supplyType]);
    int row = 30;
    ecoBaseManager.displayStatus(row);
    warriorTrainer.displayStatus(row);
    buildingConstructer.displayStatus(row);
    squadCommander.displayStatus(row);
    cartographer.displayStatus(row);
    displayUnitInfo();
}

#endif
