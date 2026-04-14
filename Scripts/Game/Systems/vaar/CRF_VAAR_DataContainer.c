enum CRF_VAAR_EEventTypes
{
	KILL
}

// This represents a AI or Player position
[BaseContainerProps()]
class CRF_VAAR_EntitySnapshot : Managed
{
    RplId entityID;
    string entityName;
    float posX, posY, posZ;
    float yaw;
    
    void CRF_VAAR_EntitySnapshot(RplId id, string name, vector pos, vector aim)
    {
		entityID = id;
        entityName = name;
        posX = pos[0];
        posY = pos[1];
        posZ = pos[2];
        yaw = aim[0];
    }
}

// This reperesents shot being fired by a player or AI
[BaseContainerProps()]
class CRF_VAAR_ShotEvent : Managed
{
    RplId shooterID;
    float startX, startZ, shotHitX, shotHitZ;
    
    void CRF_VAAR_ShotEvent(RplId id, vector start, float hitX, float hitZ)
    {
        shooterID = id;
        startX = start[0]; 
		startZ = start[2];
        shotHitX = hitX; 
		shotHitZ = hitZ;
    }
}

// This reperesents events witht type (CRF_VAAR_EEventTypes)
[BaseContainerProps()]
class CRF_VAAR_Event : Managed
{
    CRF_VAAR_EEventTypes type;
    RplId targetID, instigatorID;
    
    void CRF_VAAR_Event(CRF_VAAR_EEventTypes eventType, RplId target, RplId instigator)
    {
        type = type;
		targetID = target;
		instigatorID = instigator;
    }
}

// A single frame
[BaseContainerProps()]
class CRF_VAAR_Frame : Managed
{
    float Timestamp;
    ref array<ref CRF_VAAR_EntitySnapshot> Entities = {};
	ref array<ref CRF_VAAR_ShotEvent> Shots = {};
	ref array<ref CRF_VAAR_Event> Events = {}; // TODO
}