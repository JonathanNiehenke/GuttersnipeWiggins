#pragma once
#include <BWAPI.h>

class GW : public BWAPI::AIModule
{
public:
    virtual void onStart();
    virtual void onFrame();
    virtual void onUnitCreate(BWAPI::Unit unit);
    virtual void onUnitMorph(BWAPI::Unit unit);
    virtual void onUnitComplete(BWAPI::Unit unit);
    virtual void onUnitDestroy(BWAPI::Unit unit);
    virtual void onUnitDiscover(BWAPI::Unit unit);
    virtual void onUnitEvade(BWAPI::Unit unit);
    virtual void onUnitShow(BWAPI::Unit unit);
    virtual void onUnitHide(BWAPI::Unit unit);
    virtual void onUnitRenegade(BWAPI::Unit unit);
    virtual void onNukeDetect(BWAPI::Position target);
    virtual void onSendText(std::string text);
    virtual void onReceiveText(BWAPI::Player player, std::string text);
    virtual void onSaveGame(std::string gameName);
    virtual void onPlayerLeft(BWAPI::Player player);
    virtual void onEnd(bool isWinner);

private:
    void GW::manageProduction();
    void GW::manageAttackGroups();
    void GW::combatMicro();
    std::set<BWAPI::TilePosition> collectScoutingLocations();
    std::vector<BWAPI::TilePosition> getMineralClusterLocations();
    void constructUnit(BWAPI::UnitType constructableType);
    int getAvailableSupply();
    int getUnitBuffer(BWAPI::UnitType);
    void scout();
    void scoutLocations(std::vector<BWAPI::TilePosition> mineralLocations);
    void attackLocations(
        BWAPI::Unitset, std::vector<BWAPI::TilePosition> mineralLocations);
    void GW::appendAttackers();
    void GW::attackEnemy(BWAPI::Unitset Attackers);
    void GW::attackUnit(BWAPI::Unitset Attackers, BWAPI::Unit targetUnit);
    void GW::removeLocation(BWAPI::TilePosition Location);
    void GW::removeLocation(BWAPI::Player Player, BWAPI::TilePosition Location);
    void displayState();
};
