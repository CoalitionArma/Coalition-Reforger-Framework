//------------------------------------------------------------------------------------
// EXAMPLE: How to use CRF_MortarStrikeController (Zeus-Style Projectile System)
// 
// This controller uses the EXACT SAME system as Zeus/Game Master (SCR_EffectsModuleComponent):
// - Spawns actual mortar PROJECTILES at elevated positions (not instant explosions)
// - Projectiles fly through air with realistic ballistics
// - Auto-detonate on ground impact
// - Includes incoming whistle sounds and visible trajectories
// 
// Copy/paste these examples into your code as needed
//------------------------------------------------------------------------------------

//===================================================================================
// EXAMPLE 1: Basic Usage - Spawn strikes at a position
//===================================================================================
void Example_BasicMortarStrikes(vector position)
{
	// Create controller instance
	CRF_MortarStrikeController controller = new CRF_MortarStrikeController();
	
	// Trigger strikes (uses default settings: 10 strikes, 100m radius)
	controller.TriggerStrikes(position);
}

//===================================================================================
// EXAMPLE 1b: Basic Usage WITH DEBUG MODE (Recommended for testing)
//===================================================================================
void Example_BasicMortarStrikesWithDebug(vector position)
{
	// Create controller with debug mode enabled
	CRF_MortarStrikeController controller = new CRF_MortarStrikeController();
	controller.SetDebugMode(true); // Enable debug logging
	
	// Trigger strikes - watch console for debug messages
	controller.TriggerStrikes(position);
	
	// You should see messages like:
	// [CRF_MortarStrike] Strikes triggered at position...
	// [CRF_MortarStrike] Warning broadcasted
	// [CRF_MortarStrike] Strike 1/10 spawned at...
}

//===================================================================================
// EXAMPLE 2: Customized Strikes (Using Setters)
//===================================================================================
void Example_CustomizedMortarStrikes(vector position)
{
	// Create controller
	CRF_MortarStrikeController controller = new CRF_MortarStrikeController();
	
	// Customize settings
	controller.SetStrikeCount(15);                    // 15 strikes
	controller.SetStrikeRadius(150);                  // 150 meter radius
	controller.SetStrikeDelay(0.6);                   // 0.6 seconds between strikes
	controller.SetNotificationMessage("Artillery bombardment incoming!");
	
	// Use 82mm mortar projectile (Zeus-style)
	controller.SetProjectilePrefab("{9BC83A97341695D7}Prefabs/Weapons/Ammo/Ammo_Projectile_M821A2_82mm_HE.et");
	controller.SetProjectileSpawnHeight(200);         // Spawn 200m above ground
	
	// Trigger
	controller.TriggerStrikes(position);
}

//===================================================================================
// EXAMPLE 2b: Customized Strikes (Using Constructor)
//===================================================================================
void Example_CustomizedMortarStrikesWithConstructor(vector position)
{
	// Configure everything in the constructor
	CRF_MortarStrikeController controller = new CRF_MortarStrikeController(
		15,    // strikeCount
		150,   // strikeRadius
		0.6,   // strikeDelay
		0.5,   // strikeDelayVariation
		"{9BC83A97341695D7}Prefabs/Weapons/Ammo/Ammo_Projectile_M821A2_82mm_HE.et", // projectilePrefab
		200.0, // projectileSpawnHeight
		true,  // playWarningSound
		"{6A5000BE907EFD34}Sounds/Vehicles/Helicopters/Mi-8MT/Samples/WarningVoiceLines/Vehicles_Mi-8MT_WarningBeep_LP.wav", // warningSound
		2.0,   // warningDuration
		true,  // showNotification
		"Artillery bombardment incoming!", // notificationMessage
		false  // debugMode
	);
	
	// Trigger
	controller.TriggerStrikes(position);
}

//===================================================================================
// EXAMPLE 3: For Rush Gamemode - Add to CRF_Rush_Game.c
//===================================================================================
// Add this to your MCOMDestroyed method:

/*
protected void MCOMDestroyed(string mcomIdentifier) 
{
	if (!Replication.IsServer())
		return;
	
	// ... your existing code ...
	
	IEntity mcomEntity = GetMCOMEntity(mcomIdentifier);
	if (mcomEntity)
	{
		// Get MCOM position
		vector mcomPosition = mcomEntity.GetOrigin();
		
		// Create and configure mortar strikes
		CRF_MortarStrikeController mortarStrikes = new CRF_MortarStrikeController();
		mortarStrikes.SetStrikeCount(10);
		mortarStrikes.SetStrikeRadius(100);
		mortarStrikes.TriggerStrikes(mcomPosition);
		
		// ... rest of your existing code ...
	}
}
*/

//===================================================================================
// EXAMPLE 4: Zone-Based Progressive Intensity
//===================================================================================
void Example_ZoneBasedStrikes(int zoneNumber, vector position)
{
	CRF_MortarStrikeController controller = new CRF_MortarStrikeController();
	
	// Scale based on zone
	controller.SetStrikeCount(5 + (zoneNumber * 3));      // Zone 1: 8, Zone 2: 11, Zone 3: 14
	controller.SetStrikeRadius(75 + (zoneNumber * 25));   // Zone 1: 100m, Zone 2: 125m, Zone 3: 150m
	
	// Different projectile types per zone (Zeus-style)
	switch (zoneNumber)
	{
		case 1:
			// Light: 60mm mortar projectile
			controller.SetProjectilePrefab("{E30305B0DFC38E5E}Prefabs/Weapons/Ammo/Ammo_Projectile_M720A1_60mm_HE.et");
			controller.SetProjectileSpawnHeight(150); // Lower/faster
			controller.SetNotificationMessage("Light mortar fire incoming!");
			break;
			
		case 2:
			// Standard: 82mm mortar projectile
			controller.SetProjectilePrefab("{9BC83A97341695D7}Prefabs/Weapons/Ammo/Ammo_Projectile_M821A2_82mm_HE.et");
			controller.SetProjectileSpawnHeight(200); // Standard height
			controller.SetNotificationMessage("Mortar bombardment incoming!");
			break;
			
		case 3:
			// Heavy: 82mm with higher spawn (larger arc)
			controller.SetProjectilePrefab("{9BC83A97341695D7}Prefabs/Weapons/Ammo/Ammo_Projectile_M821A2_82mm_HE.et");
			controller.SetProjectileSpawnHeight(250); // Higher/dramatic
			controller.SetNotificationMessage("DANGER! Heavy artillery incoming!");
			controller.SetWarningDuration(3.0);  // Give more warning for heavy strikes
			break;
	}
	
	controller.TriggerStrikes(position);
}

//===================================================================================
// EXAMPLE 5: Silent Surprise Attack (No Warning)
//===================================================================================
void Example_SilentStrikes(vector position)
{
	CRF_MortarStrikeController controller = new CRF_MortarStrikeController();
	
	// Disable warnings
	controller.SetPlayWarningSound(false);
	controller.SetShowNotification(false);
	controller.SetWarningDuration(0);  // Immediate strikes
	
	// Quick, deadly strikes
	controller.SetStrikeCount(20);
	controller.SetStrikeDelay(0.3);  // Fast strikes
	
	controller.TriggerStrikes(position);
}

//===================================================================================
// EXAMPLE 6: Sequential Barrages (Creeping Barrage)
//===================================================================================
void Example_CreepingBarrage()
{
	// Define barrage positions (moving line)
	array<vector> positions = {
		"5000 100 5000",
		"5050 100 5000",
		"5100 100 5000",
		"5150 100 5000"
	};
	
	// Trigger each position with delay
	for (int i = 0; i < positions.Count(); i++)
	{
		int delay = i * 8000; // 8 seconds apart
		GetGame().GetCallqueue().CallLater(SpawnBarrageAtPosition, delay, false, positions[i]);
	}
}

void SpawnBarrageAtPosition(vector position)
{
	CRF_MortarStrikeController controller = new CRF_MortarStrikeController();
	controller.SetStrikeCount(8);
	controller.SetStrikeRadius(60);
	controller.TriggerStrikes(position);
}

//===================================================================================
// EXAMPLE 7: Random Strikes Across Multiple Points
//===================================================================================
void Example_MultiPointStrikes(array<vector> targetPositions)
{
	// Strike each position
	foreach (vector pos : targetPositions)
	{
		CRF_MortarStrikeController controller = new CRF_MortarStrikeController();
		controller.SetStrikeCount(5);
		controller.SetStrikeRadius(50);
		controller.TriggerStrikes(pos);
	}
}

//===================================================================================
// EXAMPLE 8: Delayed Strike with Custom Setup
//===================================================================================
void Example_DelayedStrike(vector position, float delaySeconds)
{
	// Schedule strike for later
	GetGame().GetCallqueue().CallLater(ExecuteDelayedStrike, delaySeconds * 1000, false, position);
}

void ExecuteDelayedStrike(vector position)
{
	CRF_MortarStrikeController controller = new CRF_MortarStrikeController();
	controller.SetStrikeCount(12);
	controller.SetStrikeRadius(120);
	controller.TriggerStrikes(position);
	
	Print(string.Format("[Example] Executed delayed mortar strike at %1", position));
}
