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

#include "plexe/apps/SimplePlatooningApp.h"

#include "plexe/protocols/BaseProtocol.h"
#include "plexe/scenarios/BaseScenario.h"

#include "veins/modules/mac/ieee80211p/Mac1609_4.h"
#include "plexe/messages/NewFormation_m.h"
#include "plexe/messages/PlexeInterfaceControlInfo_m.h"

using namespace veins;

namespace plexe {

Define_Module(SimplePlatooningApp);

void SimplePlatooningApp::initialize(int stage)
{
    BaseApp::initialize(stage);

    if (stage == 1) {
        // connect maneuver application to protocol
        protocol->registerApplication(MANEUVER_TYPE, gate("lowerLayerIn"),
            gate("lowerLayerOut"), gate("lowerControlIn"),
            gate("lowerControlOut"));
        // register to the signal indicating failed unicast transmissions
        findHost()->subscribe(veins::Mac1609_4::sigRetriesExceeded, this);
        scenario = FindModule<BaseScenario*>::findSubModule(getParentModule());
    }
}

AbandonPlatoon* SimplePlatooningApp::createAbandonMessage()
{
    AbandonPlatoon* abmsg = new AbandonPlatoon();
    abmsg->setVehicleId(positionHelper->getId());
    abmsg->setPlatoonId(positionHelper->getPlatoonId());
    abmsg->setDestinationId(positionHelper->getLeaderId());
    abmsg->setExternalId(positionHelper->getExternalId().c_str());
    abmsg->setKind(MANEUVER_TYPE);
    return abmsg;
}

NewFormation* SimplePlatooningApp::createNewFormationMessage(const std::vector<int>& newPlatoonFormation)
{
    NewFormation* nfmsg = new NewFormation();
    nfmsg->setKind(MANEUVER_TYPE);
    nfmsg->setPlatoonFormationArraySize(newPlatoonFormation.size());
    for (unsigned int i = 0; i < newPlatoonFormation.size(); i++) {
        nfmsg->setPlatoonFormation(i, newPlatoonFormation[i]);
    }
    return nfmsg;

}

void SimplePlatooningApp::sendAbandonMessage()
{
    getSimulation()->getActiveEnvir()->alert("Sending an abandon message");
    AbandonPlatoon* abmsg = createAbandonMessage();
    sendUnicast(abmsg, abmsg->getDestinationId());
}

void SimplePlatooningApp::sendNewFormationToFollowers(const std::vector<int>& newPlatoonFormation)
{
    NewFormation* nfmsg = createNewFormationMessage(newPlatoonFormation);
    int dest;
    // send a copy to each platoon follower
    for (int i = 1; i < newPlatoonFormation.size(); i++) {
        dest = newPlatoonFormation[i];
        NewFormation* dup = nfmsg->dup();
        dup->setDestinationId(dest);
        sendUnicast(dup, dest);
    }
    delete nfmsg;

}

void SimplePlatooningApp::sendUnicast(cPacket* msg, int destination)
{
    Enter_Method_Silent();
    take(msg);
    BaseFrame1609_4* frame = new BaseFrame1609_4("BaseFrame1609_4",
            msg->getKind());
    frame->setRecipientAddress(destination);
    frame->setChannelNumber(static_cast<int>(Channel::cch));
    frame->encapsulate(msg);
    // send unicast frames using 11p only
    PlexeInterfaceControlInfo* ctrl = new PlexeInterfaceControlInfo();
    ctrl->setInterfaces(PlexeRadioInterfaces::VEINS_11P);
    frame->setControlInfo(ctrl);
    sendDown(frame);
}

void SimplePlatooningApp::handleAbandonPlatoon(const AbandonPlatoon* msg)
{
    if (msg->getPlatoonId() != positionHelper->getPlatoonId())
        return;
    // only leader listens to AbandonMessages
    if (msg->getDestinationId() != positionHelper->getLeaderId())
        return;
    if (msg->getDestinationId() != positionHelper->getId())
        return;

    // Retrieving relevant info from Abandon Message
    int leaderID, leaverID, platoonID;
    leaderID = positionHelper->getId();
    leaverID = msg->getVehicleId();
    platoonID = msg->getPlatoonId();

    // Informing SUMO via Plexe Interface to remove vehicle from platoon
    plexeTraciVehicle->removePlatoonMember(msg->getExternalId());

    // Changing platoon Formation...
    std::vector<int> formation = positionHelper->getPlatoonFormation();

    // Removing the vehicle that abandoned the platoon
    formation.pop_back();
    positionHelper->setPlatoonFormation(formation);

    char text[250];
    sprintf(text, "LEADER[%d]: I'm removing v<%d> from platoon<%d>\n", leaderID, leaverID, platoonID);
    LOG << text << endl;
    getSimulation()->getActiveEnvir()->alert(text);

    sendNewFormationToFollowers(formation);
}

void SimplePlatooningApp::handleNewFormation(const NewFormation* msg)
{
    std::vector<int> newFormation;
    for (int i = 0; i < msg->getPlatoonFormationArraySize(); i++)
        newFormation.push_back(msg->getPlatoonFormation(i));

    std::string formationString = "[ ";
    for (int i = 0; i < newFormation.size(); i++) {
        formationString += std::to_string(newFormation[i]) + " ";
    }
    formationString += "]";

    char text[250];
    sprintf(text, "v<%d> got newFormation = %s\n", positionHelper->getId(), formationString.c_str());
    getSimulation()->getActiveEnvir()->alert(text);

    positionHelper->setPlatoonFormation(newFormation);
}

void SimplePlatooningApp::handleLowerMsg(cMessage* msg)
{
    BaseFrame1609_4* frame = check_and_cast<BaseFrame1609_4*>(msg);

    cPacket* enc = frame->getEncapsulatedPacket();
    ASSERT2(enc, "received a BaseFrame1609_4s with nothing inside");

    if (enc->getKind() == MANEUVER_TYPE) {
        ManeuverMessage* mm = check_and_cast<ManeuverMessage*>(
            frame->decapsulate());
        if (AbandonPlatoon* msg = dynamic_cast<AbandonPlatoon*>(mm)) {
            handleAbandonPlatoon(msg);
            delete msg;
        }
        else if (NewFormation* msg = dynamic_cast<NewFormation*>(mm)) {
            handleNewFormation(msg);
            delete msg;
        }
        delete frame;
    }
    else {
        BaseApp::handleLowerMsg(msg);
    }
}


} // namespace plexe
