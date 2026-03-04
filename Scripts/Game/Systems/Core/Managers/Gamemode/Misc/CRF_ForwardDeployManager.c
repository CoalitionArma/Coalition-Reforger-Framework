class CRF_ForwardDeployRequest
{
	int m_iPlayerId;
	vector m_vTransform;
}

class CRF_ForwardDeployManagerClass : ScriptComponentClass {}

class CRF_ForwardDeployManager : ScriptComponent
{	
	protected ref array<IEntity> m_aForwardDeployZones = {};
	protected ref array<ref CRF_ForwardDeployRequest> m_aForwardDeployRequests = {};
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 ONFRAME METHOD
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//Needed so when we teleport players/vehicles the aren't spawning on top of each other.
	float m_fBuffer = 0;
	override void EOnFrame(IEntity owner, float timeSlice)
	{
	    super.EOnFrame(owner, timeSlice);
	    m_fBuffer += timeSlice;
	    if (m_fBuffer > 0.1)
	    {
	        m_fBuffer = 0;
	        if (m_aForwardDeployRequests.Count() > 0)
	        {
	            CRF_ForwardDeployRequest request = m_aForwardDeployRequests.Get(0);
	            if (request)
	            {
	                PerformForwardDeploy(request.m_iPlayerId, request.m_vTransform);
	                m_aForwardDeployRequests.RemoveOrdered(0);
	            }
	        }
	        if (m_aForwardDeployRequests.Count() == 0)
	            ClearEventMask(owner, EntityEvent.FRAME);
	    }
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 FORWARD DEPLOY ZONE METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void AddForwardDeployZone(IEntity entity)
	{
		m_aForwardDeployZones.Insert(entity);
	}
	
	//------------------------------------------------------------------------------------------------
	array<IEntity> GetForwardDeployZones()
	{
		return m_aForwardDeployZones;
	}
		
	//------------------------------------------------------------------------------------------------
	void DeleteAllForwardDeployZones()
	{
		foreach(IEntity zone : m_aForwardDeployZones)
		{
			if(zone)
				SCR_EntityHelper.DeleteEntityAndChildren(zone);
		}
		
		m_aForwardDeployZones.Clear();
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 FORWARD DEPLOY METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	void CreateForwardDeployRequest(int playerId, vector transform)
	{
		ref CRF_ForwardDeployRequest request = new CRF_ForwardDeployRequest();
		request.m_iPlayerId = playerId;
		request.m_vTransform = transform;
		m_aForwardDeployRequests.Insert(request);
		SetEventMask(GetOwner(), EntityEvent.FRAME);
	}
	
	//------------------------------------------------------------------------------------------------
	void PerformForwardDeploy(int playerId, vector transform)
	{
		vector initialSpawnLocation;
		SCR_WorldTools.FindEmptyTerrainPosition(initialSpawnLocation, transform, 10);
		vector params[4];
		params[3] = initialSpawnLocation;
		SCR_TerrainHelper.OrientToTerrain(params, GetGame().GetWorld(), true);
		vector finalSpawnLocation;
		SCR_TerrainHelper.SnapToGeometry(finalSpawnLocation, params[3], null);
		params[3] = finalSpawnLocation;
		SCR_Global.TeleportPlayer(playerId, finalSpawnLocation, SCR_EPlayerTeleportedReason.NONE);
		CRF_RplBroadcastManager.GetInstance().BroadcastVehiclePosUpdate(finalSpawnLocation, playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	/*
	* Checks if there are any forward deploy zones active for this faction.
	* These are deleted and then removed from m_aVisibleForFactions on safestart ending.
	* @param factionKey is the faction of the group you are checking.
	* @return True if there is an active forward deploy zone.
	*/
	bool IsForwardDeployActive(string factionKey)
	{
		if (m_aForwardDeployZones.Count() == 0)
			return false;
		
		bool isActive = false;
		foreach (IEntity zone: m_aForwardDeployZones)
		{
			CRF_PolyZone polyZone = CRF_PolyZone.Cast(zone.FindComponent(CRF_PolyZone));
			if (!polyZone)
				continue;

			if(!polyZone.m_aVisibleForFactions.Contains(factionKey))
				continue;
			
			isActive = true;
			break;
		}
		
		return isActive;
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	static protected CRF_ForwardDeployManager m_sInstance;
	void CRF_ForwardDeployManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_ForwardDeployManager GetInstance()
	{
		return m_sInstance;
	}
}