//
// Copyright (C) 2012-2023 Michele Segata <segata@ccs-labs.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#pragma once

#include "plexe/apps/BaseApp.h"
#include "plexe/scenarios/BaseScenario.h"
#include "plexe/messages/AbandonPlatoon_m.h"
#include "plexe/messages/NewFormation_m.h"


namespace plexe {

class SimplePlatooningApp : public BaseApp {

public:
    SimplePlatooningApp()
    {
    }

    void sendAbandonMessage();
    virtual void sendUnicast(cPacket* msg, int destination);

protected:
    /** override from BaseApp */
    virtual void initialize(int stage) override;
    virtual void handleLowerMsg(cMessage* msg) override;

    BaseScenario* scenario;

private:
    AbandonPlatoon* createAbandonMessage();
    NewFormation* createNewFormationMessage(const std::vector<int>& newPlatoonFormation);
    void handleAbandonPlatoon(const AbandonPlatoon* msg);
    void handleNewFormation(const NewFormation* msg);
    void sendNewFormationToFollowers(const std::vector<int>& newPlatoonFormation);
};


} // namespace plexe
