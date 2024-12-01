import ManeuverMessage;

// Message sent by a vehicle to notify the leader that this vehicle left the platoon.
// NB: this message contains also all fields inherited from ManeuverMessage!
// e.g., vehicleId and platoonId 
packet AbandonPlatoon extends ManeuverMessage {
	string msgContent = "Goodbye I'm leaving";
}