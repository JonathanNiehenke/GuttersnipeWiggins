#pragma once
#include <BWAPI.h>
#include "Combat.h"

namespace {

    class Squad {
        private:
            BWAPI::Unitset members;
            Combat combat;
        public:
            Squad(const BWAPI::Unit&, const BWAPI::Position&);
            BWAPI::Position getAvgPosition() const;
            bool isEmpty() const { return members.empty(); };
            void assign(const BWAPI::Unit& armyUnit);
            void remove(const BWAPI::Unit& deadArmyUnit);
            bool isJoinable(const Squad& otherSquad) const;
            void join(Squad& otherSquad);
            // void split(const int& newSquadAmount);
            void attackPosition(const BWAPI::Position& position);
            void prepareCombat();
            void engageCombat() const;
    };
}

class SquadCommander {
    private:
        std::vector<Squad> deployedForces;
        void uniteNearBySquads();
        void removeEmptySquads();
    public:
        void commandSquadsTo(const BWAPI::Position& attackPosition);
        void sendUnitToAttack(const BWAPI::Unit&, const BWAPI::Position&);
        void removeFromDuty(const BWAPI::Unit& armyUnit);
        void updateGrouping();
        void updateTargeting();
        void updateAttacking();
        void drawStatus(int& row) const;
};
