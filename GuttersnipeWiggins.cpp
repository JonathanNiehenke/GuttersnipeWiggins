#pragma once
#include "GuttersnipeWiggins.h"

using namespace BWAPI::Filter;

GW::GW() {
    cartographer = new Cartographer();
    production = nullptr;  // Initalized in onStart
    squadCommander = new SquadCommander(std::bind(
        &Cartographer::nextPosition, cartographer, std::placeholders::_1));
    decisionSequence = new DecisionSequence();
}

GW::~GW() {
    delete cartographer;
    // delete production;  Causes crash, but I don't undestand why.
    delete squadCommander;
    delete decisionSequence;
}

void GW::onStart()
{
    BWAPI::Broodwar->enableFlag(1);  // Enabled for debugging.
    Self = BWAPI::Broodwar->self();
    cartographer->discoverResourcePositions();
    switch (Self->getRace()) {
        case BWAPI::Races::Enum::Protoss:
            production = new ProtossProduction();
            break;
        case BWAPI::Races::Enum::Terran:
            production = new TerranProduction();
            break;
        case BWAPI::Races::Enum::Zerg:
            production = new ZergProduction();
            break;
    }
    decisionSequence->onStart(production);
}

void GW::onFrame()
{
    const int actionFrames = std::max(5, BWAPI::Broodwar->getLatency());
    GW::drawStatus();  // For debugging.
    switch(BWAPI::Broodwar->getFrameCount() % actionFrames) {
        case 0: decisionSequence->update();
            break;
        case 1: production->update();
            break;
        case 2: cartographer->update();
            break;
        case 3:
            if (cartographer->lacksEnemySighting())
                squadCommander->search(cartographer->searchPositions());
            else
                squadCommander->charge();
            break;
        case 4:
            squadCommander->execute();
        default: break;
    }
}

void GW::onUnitCreate(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() == Self) {
        production->onUnitCreate(Unit);
    }
}

void GW::onUnitMorph(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() == Self)
        production->onUnitMorph(Unit);
    else if (Unit->getType())
        cartographer->removeUnit(Unit);
}

void GW::onUnitComplete(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() == Self) {
        production->onUnitComplete(Unit);
        if (Unit->getType() == production->getArmyUnitType()) {
            BWAPI::Unitset temp;
            temp.insert(Unit);
            squadCommander->incorporate(temp);
        }
    }
}

void GW::onUnitDestroy(BWAPI::Unit Unit)
{
    if (Unit->getPlayer() == Self) {
        production->onUnitDestroy(Unit);
        squadCommander->deactivate(Unit);
    }
    else
        cartographer->removeUnit(Unit);
}

void GW::onUnitDiscover(BWAPI::Unit Unit) {
    if (Self->isEnemy(Unit->getPlayer()))
        cartographer->addUnit(Unit);
}

void GW::onUnitEvade(BWAPI::Unit Unit) {
    // I don't understand the purpose/use when given the unit is only
    // accessable if it were ours. As such it is a poor duplicate of
    // onUnitDestroyed. Cartographer->update() preforms this function.
}

void GW::onUnitShow(BWAPI::Unit Unit)
{
}

void GW::onUnitHide(BWAPI::Unit Unit)
{
}

void GW::onUnitRenegade(BWAPI::Unit Unit)
{
}

void GW::onNukeDetect(BWAPI::Position target)
{
}

void GW::onSendText(std::string text)
{
    if (text == "getError") {
        BWAPI::Broodwar << BWAPI::Broodwar->getLastError() << std::endl;
        return;
    }
    else if (text.substr(0, 7) == "getUnit") {
        int unitId = atoi(text.substr(8, 4).c_str());
        BWAPI::Unit unit = BWAPI::Broodwar->getUnit(unitId);
        if (unit) {
            BWAPI::Broodwar << "Found Unit: %d " << unitId << std::endl;
            BWAPI::Broodwar->setScreenPosition(unit->getPosition());
        }
        return;
    }
    // else if (text.substr(0, 6) == "getJob") {
        // int unitId = atoi(text.substr(8, 4).c_str());
        // BWAPI::Unit unit = BWAPI::Broodwar->getUnit(unitId);
    // }
    BWAPI::Unitset selectedUnits = BWAPI::Broodwar->getSelectedUnits();
    if (text == "isStuck") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Broodwar->sendTextEx(true, "%d: %s",
                unit->getID(), unit->isStuck() ? "True" : "False");
            }
    }
    else if (text == "getPos") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::Position Pos = unit->getPosition();
            BWAPI::Broodwar->sendTextEx(true, "%d: (%d, %d)",
                unit->getID(), Pos.x, Pos.y);
        }
    }
    else if (text == "getLoc") {
        for (BWAPI::Unit unit: selectedUnits) {
            BWAPI::TilePosition Pos = unit->getTilePosition();
            BWAPI::Broodwar->sendTextEx(true, "%d: (%d, %d)",
                unit->getID(), Pos.x, Pos.y);
        }
    }
    else if (text == "getTargetPos") {
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
}

void GW::onEnd(bool IsWinner)
{
    delete production;
}

void GW::drawStatus() const {
    // May reveal logic errors and bugs.
    BWAPI::Broodwar->drawTextScreen(3, 3, "APM %d, FPS %d, avgFPS %f",
        BWAPI::Broodwar->getAPM(), BWAPI::Broodwar->getFPS(),
        BWAPI::Broodwar->getAverageFPS());
    int row = 20;
    cartographer->drawStatus();
    squadCommander->drawStatus(row);
    drawSelectedUnitInfo();
}

void GW::drawSelectedUnitInfo() const {
    int row = 20;
    for (BWAPI::Unit unit: BWAPI::Broodwar->getSelectedUnits()) {
        int sinceCommandFrame = (BWAPI::Broodwar->getFrameCount() -
            unit->getLastCommandFrame());
        BWAPI::Broodwar->drawTextScreen(
            440, row, "%s: %d", unit->getType().c_str(), unit->getID());
        BWAPI::Broodwar->drawTextScreen(
            440, row + 10, "  Order: %s", unit->getOrder().c_str());
        BWAPI::Broodwar->drawTextScreen(440, row + 20, "  Command: %s - %d",
            unit->getLastCommand().getType().c_str(), sinceCommandFrame);
        row += 35;
    }
}


