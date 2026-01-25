class CRF_CameraManagerClass : ScriptComponentClass
{
}

class CRF_CameraManager : ScriptComponent
{
	IEntity m_eCamera;                      // Stores local camera entity for spectator mode
	protected vector m_vStoredCameraPos[4];   // Stores camera transform between sessions
	protected vector m_vGenericSpawn[4];   // Stores camera transform between sessions
	
	protected static CRF_CameraManager m_sInstance;
	
	//------------------------------------------------------------------------------------------------
	// STATIC ACCESSORS
	//------------------------------------------------------------------------------------------------

	/**
	 * Returns the instance of this component from the player controller
	 * @return CRF_CameraManager - The camera manager component instance or null if unavailable
	 */
	
	void CRF_CameraManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
	
	static CRF_CameraManager GetInstance()
	{
		return m_sInstance;
	}
	
	//------------------------------------------------------------------------------------------------
	// METHODS
	//------------------------------------------------------------------------------------------------
	
	//------------------------------------------------------------------------------------------------
	/*!
	 * Updates stored camera position for persistence between sessions
	 * @param cameraPosToStore - Array of 4 vectors representing camera transform
	 */
	void UpdateStoredCameraPos(vector cameraPosToStoreOne, vector cameraPosToStoreTwo, vector cameraPosToStoreThree, vector cameraPosToStoreFour)
	{
		m_vStoredCameraPos[0] = cameraPosToStoreOne;
		m_vStoredCameraPos[1] = cameraPosToStoreTwo;
		m_vStoredCameraPos[2] = cameraPosToStoreThree;
		m_vStoredCameraPos[3] = cameraPosToStoreFour;
	}
	
	//------------------------------------------------------------------------------------------------
	/*!
	 * Initilizes players if they have a valid spectator entity
	 */
	void InitilizeInitialCamera()
	{
		vector cameraPos[4];
		cameraPos = SCR_PlayerController.Cast(GetGame().GetPlayerController()).m_vPlayersLastDeath;
		
		//If Respawns are enabled, everybody goes to the fixed spectator position
		if (CRF_RespawnManager.GetInstance().m_bCurrentRespawnEnabled)
			cameraPos[3] = Vector(0, 500, 0);
		// Use provided death position if available
		else if (CRF_GamemodeManager.IsValidSpawnVector(cameraPos[3])) {
			cameraPos[3][1] = cameraPos[3][1] + 1.5; // Elevate camera slightly above death position
		}
		// Use stored camera position if available
		else if (CRF_GamemodeManager.IsValidSpawnVector(m_vStoredCameraPos[3])) {
			cameraPos = m_vStoredCameraPos;
		} 
		// Fallback to generic spawn position
		else {
			cameraPos = m_vGenericSpawn;
		}
			
		// Set up camera entity
		EntitySpawnParams cameraSpawnParams = new EntitySpawnParams();
		cameraSpawnParams.TransformMode = ETransformMode.WORLD;
		cameraSpawnParams.Transform = cameraPos;

		// Spawn or reposition camera
		if (!m_eCamera)
			m_eCamera = GetGame().SpawnEntityPrefab(Resource.Load("{E1FF38EC8894C5F3}Prefabs/Editor/Camera/ManualCameraSpectate.et"), GetGame().GetWorld(), cameraSpawnParams);
		else
			m_eCamera.SetWorldTransform(cameraPos);
		
		// Level camera horizon
		vector mat = m_eCamera.GetAngles();
		m_eCamera.SetAngles(Vector(mat[0], mat[1], 0));
		
		// Switch to spectator camera
		GetGame().GetCameraManager().SetCamera(CameraBase.Cast(m_eCamera));
	};
}