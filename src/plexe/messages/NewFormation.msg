import ManeuverMessage;

// Message sent by a platoon leader to notify all followers
// about a change in the platoon formation
packet NewFormation extends ManeuverMessage {
	int platoonFormation[];
}
