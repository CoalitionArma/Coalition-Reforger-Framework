//------------------------------------------------------------------------------------
// CRF_MortarStrikeController: Utility class for spawning mortar strikes
// 
// TWO METHODS AVAILABLE:
// 1. TriggerStrikes() - Direct projectile spawn (RECOMMENDED)
//    - Spawns projectiles directly at 200m altitude
//    - Works out of the box, no prefab creation needed
//    - Requires projectile prefab with SCR_ShellSoundComponent for whistling sound
//    - Full control over strike parameters
// 
// 2. TriggerStrikesWithEffectsModule() - Zeus-style effects module (ADVANCED)
//    - Uses SCR_EffectsModuleComponent exactly like Zeus/Game Master
//    - Requires creating a custom effects module prefab in Workbench first
//    - Base game Zeus prefabs are not accessible from mods
//    - More complex setup but allows using existing Zeus configurations
//
// SOUND SETUP:
// The whistling/streaking sound is handled by SCR_ShellSoundComponent on the projectile.
// Your projectile prefab needs:
// - SCR_ShellSoundComponent with UpdateSoundJob enabled
// - SignalsManagerComponent with signals: Speed, SpeedVertical, DistanceToClosestPoint, CosAngleProjectileToListener
// - Sound events configured in the projectile's SoundComponent
//
// Spawn instances dynamically when needed (e.g., when MCOM is destroyed)
//------------------------------------------------------------------------------------

class CRF_MortarStrikeController
{
	protected SCR_EffectsModuleComponent m_EffectsModule;
	//===================================================================================
	// CONFIGURATION PROPERTIES
	//===================================================================================
	
	protected int m_iStrikeCount = 10;
	protected float m_fStrikeRadius = 100;
	protected float m_fStrikeDelay = 0.8;
	protected float m_fStrikeDelayVariation = 0.5;
	
	// Zeus-style projectile prefab (actual mortar round that flies)
	// NOTE: Must have SCR_ShellSoundComponent + SignalsManagerComponent for whistling sound!
	protected ResourceName m_rProjectilePrefab = "{98EC9C526AFBA282}Prefabs/Weapons/Ammo/Ammo_Shell_82mm_HE_O832DU.et";
	
	// Spawn height for projectiles (meters above ground)
	protected float m_fProjectileSpawnHeight = 200.0;
	
	protected bool m_bPlayWarningSound = true;
	protected ResourceName m_rWarningSound = "{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav";
	protected float m_fWarningDuration = 2.0;
	protected bool m_bShowNotification = true;
	protected string m_sNotificationMessage = "Incoming mortar strikes!";
	protected bool m_bDebugMode = false;
	
	//===================================================================================
	// RUNTIME VARIABLES
	//===================================================================================
	
	protected bool m_bStrikesActive = false;
	protected int m_iStrikesSpawned = 0;
	protected vector m_vCenterPosition;
	protected IEntity m_OwnerEntity; // Virtual owner entity for projectiles
	
	// Keep reference to prevent garbage collection during CallLater
	protected static ref array<ref CRF_MortarStrikeController> s_ActiveControllers = new array<ref CRF_MortarStrikeController>();
	
	//===================================================================================
	// CONSTRUCTOR
	//===================================================================================
	
	/**
	 * Constructor - configure all settings at once or use defaults
	 * @param strikeCount Number of strikes to spawn (default: 10)
	 * @param strikeRadius Radius in meters for strike distribution (default: 100)
	 * @param strikeDelay Base delay between strikes in seconds (default: 0.8)
	 * @param strikeDelayVariation Random variation to add to delay +/- (default: 0.5)
	 * @param projectilePrefab Prefab resource path for mortar projectile (default: 82mm HE)
	 * @param projectileSpawnHeight Height above ground to spawn projectiles in meters (default: 200)
	 * @param playWarningSound Enable warning sound (default: true)
	 * @param warningSound Custom warning sound resource path (default: beep sound)
	 * @param warningDuration Duration of warning period in seconds (default: 2.0)
	 * @param showNotification Enable popup notification (default: true)
	 * @param notificationMessage Custom notification message (default: "Incoming mortar strikes!")
	 * @param debugMode Enable debug logging (default: false)
	 * 
	 * Examples:
	 *   new CRF_MortarStrikeController(); // Uses all defaults
	 *   new CRF_MortarStrikeController(15, 150); // 15 strikes, 150m radius, rest defaults
	 *   new CRF_MortarStrikeController(20, 200, 0.5); // Custom count, radius, and delay
	 */
	void CRF_MortarStrikeController(
		int strikeCount = 10,
		float strikeRadius = 100,
		float strikeDelay = 0.8,
		float strikeDelayVariation = 0.5,
		ResourceName projectilePrefab = "{98EC9C526AFBA282}Prefabs/Weapons/Ammo/Ammo_Shell_82mm_HE_O832DU.et",
		float projectileSpawnHeight = 200.0,
		bool playWarningSound = true,
		ResourceName warningSound = "{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav",
		float warningDuration = 2.0,
		bool showNotification = true,
		string notificationMessage = "Incoming mortar strikes!",
		bool debugMode = false
	)
	{
		m_iStrikeCount = strikeCount;
		m_fStrikeRadius = strikeRadius;
		m_fStrikeDelay = strikeDelay;
		m_fStrikeDelayVariation = strikeDelayVariation;
		m_rProjectilePrefab = projectilePrefab;
		m_fProjectileSpawnHeight = projectileSpawnHeight;
		m_bPlayWarningSound = playWarningSound;
		m_rWarningSound = warningSound;
		m_fWarningDuration = warningDuration;
		m_bShowNotification = showNotification;
		m_sNotificationMessage = notificationMessage;
		m_bDebugMode = debugMode;
	}
	
	//===================================================================================
	// PUBLIC API
	//===================================================================================
	
	/**
	 * Trigger mortar strikes using Zeus-style SCR_EffectsModuleComponent
	 * This method spawns a temporary entity with the effects module, just like Zeus does
	 * @param centerPosition Center position for the strikes
	 * @param effectsModulePrefab Path to effects module prefab (MUST be created in Workbench first!)
	 * 
	 * IMPORTANT: You must create a custom effects module prefab in Workbench with:
	 * 1. Create new prefab inheriting from GenericEntity
	 * 2. Add SCR_EffectsModuleComponent with:
	 *    - m_bExecuteOnInit = true
	 *    - m_EffectConfig = SCR_BarrageEffectsModule with:
	 *      - m_eEffectsModuleType = PROJECTILE
	 *      - m_sModuleEntityPrefab = "{98EC9C526AFBA282}Prefabs/Weapons/Ammo/Ammo_Shell_82mm_HE_O832DU.et"
	 *      - m_ModuleZoneData = SCR_EffectsModulePositionData_Radius (radius, etc)
	 *      - Barrage settings: projectile count, delays, etc.
	 * 
	 * NOTE: Base game effects module prefabs are not accessible from mods.
	 * Use TriggerStrikes() instead if you don't want to create a custom prefab.
	 */
	void TriggerStrikesWithEffectsModule(vector centerPosition, ResourceName effectsModulePrefab)
	{
		// Only run on server
		if (!Replication.IsServer())
			return;
		
		if (effectsModulePrefab == "")
		{
			Print("[CRF_MortarStrike] ERROR: No effects module prefab specified!", LogLevel.ERROR);
			return;
		}
		
		if (m_bDebugMode)
			Print(string.Format("[CRF_MortarStrike] Triggering Zeus-style effects module at %1", centerPosition));
		
		// Load the effects module prefab
		Resource effectsResource = Resource.Load(effectsModulePrefab);
		if (!effectsResource)
		{
			Print(string.Format("[CRF_MortarStrike] ERROR: Failed to load effects module prefab: %1", effectsModulePrefab), LogLevel.ERROR);
			return;
		}
		
		// Setup spawn parameters
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		
		vector transform[4];
		Math3D.MatrixIdentity4(transform);
		transform[3] = centerPosition;
		spawnParams.Transform = transform;
		
		// Spawn the effects module entity (Zeus-style)
		IEntity effectsEntity = GetGame().SpawnEntityPrefab(effectsResource, GetGame().GetWorld(), spawnParams);
		if (!effectsEntity)
		{
			Print(string.Format("[CRF_MortarStrike] ERROR: Failed to spawn effects module entity"), LogLevel.ERROR);
			return;
		}
		
		// The SCR_EffectsModuleComponent will automatically execute if m_bExecuteOnInit is true
		SCR_EffectsModuleComponent effectsComp = SCR_EffectsModuleComponent.Cast(effectsEntity.FindComponent(SCR_EffectsModuleComponent));
		if (effectsComp)
		{
			if (m_bDebugMode)
				Print("[CRF_MortarStrike] Effects module spawned and executing");
		}
		else
		{
			Print("[CRF_MortarStrike] WARNING: Spawned entity has no SCR_EffectsModuleComponent!", LogLevel.WARNING);
		}
	}
	
	/**
	 * Trigger the mortar strikes at a specific position (Direct projectile spawn method)
	 * This is the custom implementation that spawns projectiles directly
	 * @param centerPosition Center position for the strikes
	 * 
	 * NOTE: For authentic whistling sounds, your projectile prefab must have:
	 * - SCR_ShellSoundComponent configured with proper signals:
	 *   - "Speed" signal
	 *   - "SpeedVertical" signal  
	 *   - "DistanceToClosestPoint" signal
	 *   - "CosAngleProjectileToListener" signal
	 * - SignalsManagerComponent to manage the signals
	 * 
	 * The sound is driven by the projectile's velocity and proximity to the listener,
	 * creating the characteristic whistling/streaking sound as it falls.
	 */
	void TriggerStrikes(vector centerPosition)
	{
		// Only run on server
		if (!Replication.IsServer())
			return;
			
		// Check if strikes are already active
		if (m_bStrikesActive)
		{
			Print("[CRF_MortarStrike] WARNING: Strikes already active!", LogLevel.WARNING);
			return;
		}
		
		m_vCenterPosition = centerPosition;
		m_bStrikesActive = true;
		m_iStrikesSpawned = 0;
		
		// Add to active controllers to prevent garbage collection
		s_ActiveControllers.Insert(this);
		
		if (m_bDebugMode)
			Print(string.Format("[CRF_MortarStrike] Triggering %1 strikes at %2 with radius %3m", m_iStrikeCount, m_vCenterPosition, m_fStrikeRadius));
		
		// Play warning sound and show notification
		if (m_bPlayWarningSound || m_bShowNotification)
		{
			BroadcastWarning();
		}
		
		// Start spawning strikes after warning duration
		float initialDelay = m_fWarningDuration * 1000; // Convert to milliseconds
		GetGame().GetCallqueue().CallLater(SpawnStrikeSequence, initialDelay, false);
	}
	
	/**
	 * Stop all active strikes
	 */
	void StopStrikes()
	{
		m_bStrikesActive = false;
		
		if (m_bDebugMode)
			Print("[CRF_MortarStrike] Strikes stopped");
	}
	
	/**
	 * Set strike count dynamically
	 */
	void SetStrikeCount(int count)
	{
		m_iStrikeCount = count;
	}
	
	/**
	 * Set strike radius dynamically
	 */
	void SetStrikeRadius(float radius)
	{
		m_fStrikeRadius = radius;
	}
	
	/**
	 * Set strike delay (seconds between strikes)
	 */
	void SetStrikeDelay(float delay)
	{
		m_fStrikeDelay = delay;
	}
	
	/**
	 * Set strike delay variation (random +/- variation in seconds)
	 */
	void SetStrikeDelayVariation(float variation)
	{
		m_fStrikeDelayVariation = variation;
	}
	
	/**
	 * Set projectile prefab dynamically
	 */
	void SetProjectilePrefab(ResourceName prefab)
	{
		m_rProjectilePrefab = prefab;
	}
	
	/**
	 * Set projectile spawn height dynamically (meters above ground)
	 */
	void SetProjectileSpawnHeight(float height)
	{
		m_fProjectileSpawnHeight = height;
	}
	
	/**
	 * Set warning sound dynamically
	 */
	void SetWarningSound(ResourceName sound)
	{
		m_rWarningSound = sound;
	}
	
	/**
	 * Enable/disable warning sound
	 */
	void SetPlayWarningSound(bool enable)
	{
		m_bPlayWarningSound = enable;
	}
	
	/**
	 * Enable/disable notification
	 */
	void SetShowNotification(bool enable)
	{
		m_bShowNotification = enable;
	}
	
	/**
	 * Set notification message
	 */
	void SetNotificationMessage(string message)
	{
		m_sNotificationMessage = message;
	}
	
	/**
	 * Set warning duration
	 */
	void SetWarningDuration(float duration)
	{
		m_fWarningDuration = duration;
	}
	
	/**
	 * Enable/disable debug mode
	 */
	void SetDebugMode(bool enable)
	{
		m_bDebugMode = enable;
	}
	
	//===================================================================================
	// INTERNAL METHODS
	//===================================================================================
	
	/**
	 * Broadcast warning to players
	 */
	protected void BroadcastWarning()
	{
		// Play warning sound
		if (m_bPlayWarningSound && m_rWarningSound != "")
		{
			AudioSystem.PlaySound(m_rWarningSound);
		}
		
		// Show popup notification
		if (m_bShowNotification && m_sNotificationMessage != "")
		{
			SCR_PopUpNotification popUp = SCR_PopUpNotification.GetInstance();
			if (popUp)
			{
				popUp.PopupMsg(m_sNotificationMessage, duration: m_fWarningDuration);
			}
		}
		
		if (m_bDebugMode)
			Print("[CRF_MortarStrike] Warning broadcasted");
	}
	
	/**
	 * Start spawning the strike sequence
	 */
	protected void SpawnStrikeSequence()
	{
		if (m_bDebugMode)
			Print(string.Format("[CRF_MortarStrike] SpawnStrikeSequence called - Active: %1, Spawned: %2/%3", m_bStrikesActive, m_iStrikesSpawned, m_iStrikeCount));
		
		if (!m_bStrikesActive || m_iStrikesSpawned >= m_iStrikeCount)
		{
			m_bStrikesActive = false;
			
			// Remove from active controllers - allows garbage collection
			int index = s_ActiveControllers.Find(this);
			if (index != -1)
				s_ActiveControllers.Remove(index);
			
			if (m_bDebugMode)
				Print("[CRF_MortarStrike] Strike sequence complete");
			return;
		}
		
		// Spawn a single strike
		SpawnSingleStrike();
		m_iStrikesSpawned++;
		
		// Schedule next strike with variation
		float baseDelay = m_fStrikeDelay * 1000; // Convert to milliseconds
		float variation = Math.RandomFloat(-m_fStrikeDelayVariation, m_fStrikeDelayVariation) * 1000;
		float nextDelay = Math.Max(100, baseDelay + variation); // Minimum 100ms delay
		
		GetGame().GetCallqueue().CallLater(SpawnStrikeSequence, nextDelay, false);
	}
	
	/**
	 * Spawn a single mortar strike using Zeus-style projectile system
	 * Spawns a flying projectile that will impact the target position
	 */
	protected void SpawnSingleStrike()
	{
		if (m_bDebugMode)
			Print(string.Format("[CRF_MortarStrike] SpawnSingleStrike called - Strike %1/%2", m_iStrikesSpawned + 1, m_iStrikeCount));
		
		// Get random target position at ground level
		vector targetPosition = GetRandomPositionInRadius(m_vCenterPosition, m_fStrikeRadius);
		
		// Calculate spawn position (elevated above target)
		vector spawnPosition = targetPosition;
		spawnPosition[1] = targetPosition[1] + m_fProjectileSpawnHeight;
		
		// Setup spawn parameters in WORLD space
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		
		// Create transform matrix
		vector transform[4];
		Math3D.MatrixIdentity4(transform);
		
		// Orient projectile toward target (Zeus-style LookAt)
		vector targetWorld = targetPosition;
		SCR_Math3D.LookAt(spawnPosition, targetWorld, vector.Up, transform);
		transform[3] = spawnPosition;
		
		spawnParams.Transform = transform;
		
		// Load resource and spawn the projectile (Zeus uses Resource.Load)
		Resource projectileResource = Resource.Load(m_rProjectilePrefab);
		if (!projectileResource)
		{
			Print(string.Format("[CRF_MortarStrike] ERROR: Failed to load projectile prefab: %1", m_rProjectilePrefab));
			return;
		}
		
		IEntity projectile = GetGame().SpawnEntityPrefab(projectileResource, GetGame().GetWorld(), spawnParams);
		
		if (projectile)
		{
			if (m_bDebugMode)
				Print(string.Format("[CRF_MortarStrike] Projectile %1/%2 spawned at %3 targeting %4", 
					m_iStrikesSpawned + 1, m_iStrikeCount, spawnPosition, targetPosition));
			
			// Launch projectile using ProjectileMoveComponent (Zeus method)
			ProjectileMoveComponent moveComponent = ProjectileMoveComponent.Cast(projectile.FindComponent(ProjectileMoveComponent));
			if (moveComponent)
			{
				// Calculate launch direction from spawn to target
				vector launchDirection = vector.Direction(spawnPosition, targetWorld);
				launchDirection.Normalize();
				
				// CRITICAL: Activate the projectile physics first
				Physics physics = projectile.GetPhysics();
				if (physics)
				{
					physics.SetActive(ActiveState.ACTIVE);
					
					if (m_bDebugMode)
						Print(string.Format("[CRF_MortarStrike] Physics activated for projectile"));
				}
				
				// Calculate initial velocity - projectiles need speed to fall!
				// Zeus uses a speed multiplier - we'll use a realistic mortar velocity
				float projectileSpeed = 100.0; // m/s - typical mortar round initial velocity
				vector initialVelocity = launchDirection * projectileSpeed;
				
				// Launch the projectile (Zeus-style launch)
				// Parameters: direction, parentVelocity, initSpeedCoef, projectileEntity, gunner, parentEntity, lockedTarget, weaponComponent
				moveComponent.Launch(launchDirection, initialVelocity, 1.0, projectile, null, null, null, null);
				
				if (m_bDebugMode)
					Print(string.Format("[CRF_MortarStrike] Projectile launched with direction %1, velocity %2", launchDirection, initialVelocity));
			}
			else
			{
				Print(string.Format("[CRF_MortarStrike] WARNING: Projectile spawned but has no ProjectileMoveComponent!"), LogLevel.WARNING);
			}
		}
		else
		{
			Print(string.Format("[CRF_MortarStrike] ERROR: Failed to spawn projectile prefab '%1' at %2", m_rProjectilePrefab, spawnPosition), LogLevel.ERROR);
		}
	}
	
	/**
	 * Get a random position within a radius around a center point
	 * @param center The center position
	 * @param radius The radius in meters
	 * @return Random position within the radius
	 */
	protected vector GetRandomPositionInRadius(vector center, float radius)
	{
		// Generate random angle (0-360 degrees)
		float angle = Math.RandomFloat(0, Math.PI * 2);
		
		// Generate random distance (0 to radius)
		// Use square root for uniform distribution
		float distance = Math.Sqrt(Math.RandomFloat(0, 1)) * radius;
		
		// Calculate offset
		float offsetX = Math.Cos(angle) * distance;
		float offsetZ = Math.Sin(angle) * distance;
		
		// Create new position
		vector randomPos = center;
		randomPos[0] = randomPos[0] + offsetX;
		randomPos[2] = randomPos[2] + offsetZ;
		
		// Get ground height at this position
		randomPos[1] = GetGame().GetWorld().GetSurfaceY(randomPos[0], randomPos[2]);
		
		return randomPos;
	}
}
