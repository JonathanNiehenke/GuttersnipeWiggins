#pragma once
#include <BWAPI.h>
#include "Combat.h"
#include "Utils.h"

namespace {

    class Squad {
        private:
            BWAPI::Unitset members;
            Combat combat;
            bool isJoinable(const Squad& otherSquad) const;
            Squad(const BWAPI::Unit& unit, const BWAPI::Position& position);
        public:
            Squad(const BWAPI::Unitset& units)
                : members(units), combat(units.getPosition()) {}
            BWAPI::Position getAvgPosition() const;
            bool isEmpty() const { return members.empty(); };
            void assign(const BWAPI::Unit& armyUnit);
            void remove(const BWAPI::Unit& deadArmyUnit);
            void join(Squad& otherSquad);
            std::vector<Squad> spreadTo(std::vector<BWAPI::Position> sPos);
            bool combatComplete();
            BWAPI::Position combatPosition();
            void combatPosition(const BWAPI::Position& position);
            void prepareCombat();
            void engageCombat() const;
    };
}

class SquadCommander {
    private:
        typedef std::vector<BWAPI::Position> PosSeries;
        typedef std::function<BWAPI::Position(BWAPI::Position)> PosToPos;
        PosToPos nextPosition;
        std::vector<Squad> deployedForces;
        void assignCombatPosition(Squad& squad) const;
        void funnel();
        void terminate();
    public:
        void focus(const BWAPI::Position& attackPosition);
        SquadCommander(PosToPos nextPosition) : nextPosition(nextPosition) {}
        void incorporate(const BWAPI::Unitset&);
        void deactivate(const BWAPI::Unit& armyUnit);
        void search(PosSeries searchPositions);
        void group();
        void prepare();
        void engage() const;
        void drawStatus(int& row) const;
};
