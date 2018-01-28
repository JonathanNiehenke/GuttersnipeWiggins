#ifndef SQUADCOMMANDER_H
#define SQUADCOMMANDER_H
#include <vector>
#include <utility>
#include <BWAPI.h>
#include "Cartographer.h"
#include "Utils.h"

// Helper class for deciding the best enemy units to attack.
class compareEnemyTargets
{
    private:
        BWAPI::Position sourcePosition = BWAPI::Positions::None;
        int getDamage(BWAPI::UnitType);
        int getDurability(BWAPI::Unit);
        bool isJunk(BWAPI::Unit unit, BWAPI::UnitType unitType);
    public:
        compareEnemyTargets(BWAPI::Position position)
            { sourcePosition = position; }
        compareEnemyTargets(BWAPI::TilePosition location)
            { sourcePosition = BWAPI::Position(location); }
        compareEnemyTargets(BWAPI::Unit unit)
            { sourcePosition = unit->getPosition(); }
        BWAPI::Unit operator()(BWAPI::Unit, BWAPI::Unit);
};

// Manages the groups of army units.
class SquadCommander
{
    private:
        const BWAPI::WeaponType noWeapon = BWAPI::WeaponTypes::None;
        BWAPI::Player self;
        Cartographer *cartographer;
        std::vector<BWAPI::Unitset> armySquads;
        //! See comment within removeEmptySquad() definition;
        // bool isEmptySquad(BWAPI::Unitset Squad);
        void drawSqaudTargetRange(BWAPI::Position squadPos);
        void drawBullsEye(BWAPI::Unit targetUnit);
        bool needToGroup(BWAPI::Unitset Squad, BWAPI::Position squadPos);
        void attackUnit(BWAPI::Unitset Squad, BWAPI::Unit targetUnit);
        void attackLocations(BWAPI::Unitset, std::vector<BWAPI::Position>);
        void attackPositon(BWAPI::Unitset Squad);
    public:
        SquadCommander::SquadCommander();
        void SquadCommander::onStart(Cartographer *cartographer );
        void drawSquadGather(BWAPI::Position Pos, int Range=70);
        void manageAttackGroups();
        void assembleSquads(BWAPI::Unit warrior);
        void removeWarrior(BWAPI::Unit deadWarrior);
        void uniteSquads();
        void removeEmptySquads();
        void combatMicro();
        void displayStatus(int &row);
};

#endif
