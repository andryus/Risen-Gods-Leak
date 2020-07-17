/*
 * Copyright (C) 2008-2017 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "UnitAI.h"

class Player;

enum class Specs : uint8;

class GAME_API PlayerAI : public UnitAI
{
    public:
        explicit PlayerAI(Player& player);

        void UpdateAI(uint32 diff) override = 0;
        void OnCharmed(bool /*apply*/) override { } // charm AI application for players is handled by Unit::SetCharmedBy / Unit::RemoveCharmedBy
        
        Creature* GetCharmer() const;

        // helper functions to determine player info
        // Return values range from 0 (left-most spec) to 2 (right-most spec). If two specs have the same number of talent points, the left-most of those specs is returned.
        static Specs GetPlayerSpec(Player const& who);
        bool IsPlayerRangedAttacker(Specs specc) const;

        struct TargetedSpell
        {
            Unit* target = nullptr;
            Spell* spell = nullptr;
            explicit operator bool() { return target && spell; }
        };

    protected:
        Player& me;

        bool IsRangedAttacker() const { return _isSelfRangedAttacker; }

        void DoAutoAttackIfReady();
        void DoRangedAttackIfReady();

        TargetedSpell SelectSpell();
        void DoCastAtTarget(TargetedSpell spell);

        void ShuffleRandomSpells();

    private:
        Unit* _SelectTargetForSpell(SpellInfo const* spellInfo) const;

        uint32 _GetHighestLearedRank(uint32 spellId) const;

        // Cancels all shapeshifts that the player could voluntarily cancel
        void _CancelAllShapeshifts();

        Specs const _selfSpec;
        bool _isSelfRangedAttacker;

        std::vector<uint32> _usableSpells;
        std::vector<double> _usableSpellsWeights;
};

class GAME_API SimpleCharmedPlayerAI : public PlayerAI
{
    public:
        SimpleCharmedPlayerAI(Player& player);
        void UpdateAI(uint32 diff) override;
        void OnCharmed(bool apply) override;

    protected:
        bool CanAIAttack(Unit const* who) const override;
        Unit* SelectAttackTarget() const;

        void UpdateAINoSpells(uint32 diff);

    private:
        TimeTrackerSmall _castCheckTimer;
        TimeTrackerSmall _forceAIChangeTimer;
        const uint32 _gcd;
        bool _chaseCloser;
        bool _forceFacing;
        bool _isFollowing;
};
