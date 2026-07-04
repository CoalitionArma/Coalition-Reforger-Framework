class CRF_PlayerSettingsManagerClass : ScriptComponentClass {}

class CRF_PlayerSettingsManager : ScriptComponent
{	
	// Game Performance Settings
	protected int m_iFPS = -1;              // Stored user FPS setting (-1 means uninitialized)
	protected int m_iAudioSetting = -1;     // Stored audio volume (-1 means uninitialized)	

//=============================================================================================================================================================================================================================================================================================================================================================
//	 SETTINGS METHODS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	//! Initializes audio lock by storing current volume and setting to 0
	void InitAudioLock()
	{
		m_iAudioSetting = AudioSystem.GetMasterVolume(AudioSystem.SFX);
		SetSFXVolume(0);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sets FPS limit to specified value
	//! \param[in] video - Video settings container
	//! \param[in] fps - FPS limit to set
	void SetFPS(BaseContainer video, int fps)
	{
		video.Set("MaxFps", fps);
		GetGame().UserSettingsChanged();
	}
	
	//------------------------------------------------------------------------------------------------
	//! Retrieves and stores initial user FPS setting
	//! \param[in] video - Video settings container
	void GetInitialUserFPSValue(BaseContainer video)
	{
		video.Get("MaxFps", m_iFPS);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Initializes FPS lock by storing current value and setting to 30
	void InitFPSLock()
	{
		//BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
		//GetInitialUserFPSValue(video);
		//SetFPS(video, 30);
	}
	
	//------------------------------------------------------------------------------------------------
	//! Sets SFX volume to specified level
	//! \param[in] volume - Volume level to set
	void SetSFXVolume(int volume)
	{
		AudioSystem.SetMasterVolume(AudioSystem.SFX, volume);
	}
	
	
	//------------------------------------------------------------------------------------------------
	//! Restores user settings to original values
	void ResetSettingsToStoredValues()
	{
		//BaseContainer video = GetGame().GetEngineUserSettings().GetModule("VideoUserSettings");
		//SetFPS(video, 0);
		SetSFXVolume(100);
	}
	
//=============================================================================================================================================================================================================================================================================================================================================================
//	 STATIC ACCESSORS
//=============================================================================================================================================================================================================================================================================================================================================================
	
	//------------------------------------------------------------------------------------------------
	protected static CRF_PlayerSettingsManager m_sInstance;
	void CRF_PlayerSettingsManager(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}

	//------------------------------------------------------------------------------------------------
	void ~CRF_PlayerSettingsManager()
	{
		if (m_sInstance == this)
			m_sInstance = null;
	}

	//------------------------------------------------------------------------------------------------
	static CRF_PlayerSettingsManager GetInstance()
	{
		return m_sInstance;
	}
}