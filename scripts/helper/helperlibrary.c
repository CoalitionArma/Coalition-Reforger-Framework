/****************************************************************************************
 * File Name:        helperlibrary
 * Author:           Harry
 * Date Created:     12/21/24
 * Description:      This is a file of useful scripts, designed as a reference.
 * 
 * Version History:
 * --------------------------------------------------------------------------------------
 * Version   Date        Author        Description
 * --------------------------------------------------------------------------------------
 * 1.0       12/21/24    Harry         Initial creation.
 * 
 * Notes:
 * The code below is typically in the script section of an object.
 * This does not cover game modes covered in the wiki.
 * Do not expect to copy, and paste this code into your script without modifying it.
 * As Reforger is rapidly developing, sections here may be out of date.
 ****************************************************************************************/

/****************************************************************************************
 * --------------Section Headers--------------
 * Vehicle Spawning
 * Sending a Global Message
 * Safestart End Check & Timer
 * Accessing a component
 * Object Presence Inside Control Trigger
 * Action on Vehicle Destruction
 * Setting an objective complete
 * Randomizing Spawns
 * Rush objective destructor
****************************************************************************************/




/****************************************************************************************
 * --------------Vehicle Spawning--------------
 * This is an example of how vehicles have been spawned in the past. Modded vehicles can create some
 * instability if spawned at the beginning of initialization, especially as so much else 
 * happens at EOnInit. This script delays the spawn slightly using the CallLater method,
 * and references the location of a base trigger entity named as the IEntity. It's not 
 * necessary they share the same name, but it does make things simpler. The string object takes 
 * the resource name of whatever you would like to have spawned. 
****************************************************************************************/

class spawnOpforVics_Class: SCR_BaseTriggerEntity 
{
	// user script
	string btr = "{C012BB3488BEA0C2}Prefabs/Vehicles/Wheeled/BTR70/BTR70.et";
	string ural = "{16C1F16C9B053801}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport.et";
	string ambu = "{43C4AF1EEBD001CE}Prefabs/Vehicles/Wheeled/UAZ452/UAZ452_ambulance.et";
	IEntity spawnBtr1, spawnBtr2, spawnUral1, spawnUral2, spawnAmbu1, spawnAmbu2, spawnAmbu3;
	
	void spawnthings()
	{
		spawnBtr1 = GetGame().GetWorld().FindEntityByName("spawnBtr1");
		spawnBtr2 = GetGame().GetWorld().FindEntityByName("spawnBtr2");
		spawnUral1 = GetGame().GetWorld().FindEntityByName("spawnUral1");
		spawnUral2 = GetGame().GetWorld().FindEntityByName("spawnUral2");
		spawnAmbu1 = GetGame().GetWorld().FindEntityByName("spawnAmbu1");
		spawnAmbu2 = GetGame().GetWorld().FindEntityByName("spawnAmbu2");
		spawnAmbu3 = GetGame().GetWorld().FindEntityByName("spawnAmbu3");
		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		
		spawnParams.Transform[3] = spawnBtr1.GetOrigin();		
		GetGame().SpawnEntityPrefab(Resource.Load(btr),GetGame().GetWorld(),spawnParams);
		spawnParams.Transform[3] = spawnBtr2.GetOrigin();
		GetGame().SpawnEntityPrefab(Resource.Load(btr),GetGame().GetWorld(),spawnParams);
		spawnParams.Transform[3] = spawnUral1.GetOrigin();
		GetGame().SpawnEntityPrefab(Resource.Load(ural),GetGame().GetWorld(),spawnParams);
		spawnParams.Transform[3] = spawnUral2.GetOrigin();
		GetGame().SpawnEntityPrefab(Resource.Load(ural),GetGame().GetWorld(),spawnParams);
		spawnParams.Transform[3] = spawnAmbu1.GetOrigin();
		GetGame().SpawnEntityPrefab(Resource.Load(ambu),GetGame().GetWorld(),spawnParams);
		spawnParams.Transform[3] = spawnAmbu2.GetOrigin();
		GetGame().SpawnEntityPrefab(Resource.Load(ambu),GetGame().GetWorld(),spawnParams);
		spawnParams.Transform[3] = spawnAmbu3.GetOrigin();
		GetGame().SpawnEntityPrefab(Resource.Load(ambu),GetGame().GetWorld(),spawnParams);
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		GetGame().GetCallqueue().CallLater(spawnthings, 3000, false);
	}

};

//Note if you have many things to spawn you could do something like this to reduce the
//amount of code necessary.
for (int i = 1; i <= 5; i++) 
{
	string btrName = "spawnBtr" + i.ToString();
	IEntity spawnBTR = GetGame().GetWorld().FindEntityByName(btrName);
	
	spawnParams.Transform[3] = spawnBTR.GetOrigin();
	GetGame().SpawnEntityPrefab(Resource.Load(btr),GetGame().GetWorld(),spawnParams);
}

//If you need to rotate the entity spawned, you can do so with SetYawPitchRoll.
spawnParams.Transform[3] = spawnBtr1.GetOrigin();
GetGame().SpawnEntityPrefab(Resource.Load(btr),GetGame().GetWorld(),spawnParams).SetYawPitchRoll("260 0 0");





/****************************************************************************************
 * --------------Sending a Global Message--------------
 * This script is set up with a faction control trigger to send a message to every player
 * once the keyed faction has met the control conditions, and in this scenario it does 
 * so on a query. The trigger will set m_bResult result to true once the 
 * hold conditions have been met. The m_sent variable simply prevents the message from being 
 * spammed every time the query is done, and the condition is met.
****************************************************************************************/

class factory_Control_Class: SCR_FactionControlTriggerEntity 
{
	// user script
	bool m_sent = false;

	override void OnQueryFinished(bool bIsEmpty)
	{
		super.OnQueryFinished(bIsEmpty);
		
		if (m_bResult && !m_sent)
		{
			SCR_PopUpNotification.GetInstance().PopupMsg("OPFOR has captured the factory. Blufor has 8 minutes to contest", 10);
			
			m_sent = true;
		}
		
	}

};

//Instead of the m_sent variable, you could also remove the trigger on completion 
//if it is no longer needed. This would be preferrable, to prevent unnecessary scripts
//from running on the server.
IEntity bridge_Control = GetGame().GetWorld().FindEntityByName("bridge_Control");
delete bridge_Control;





/****************************************************************************************
 * --------------Safestart End Check & Timer--------------
 * Coalition utilizes a safe start rule at the beginning of most missions to allow for 
 * each team to strategize, brief, etc. This class was built with two opposing forces 
 * in mind, each competing over a faction control trigger. If the defending faction held
 * the trigger for the timer duration, something happens for them. If the attacking
 * faction captured the trigger within the timer duration, something happens for them. 
 * Here is the order of the events happening.
 * 1. First, the script starts the loop checking at EOnInit if safe start has ended. 
 *      If not it re-calls the script and continues to loop.
 * 2. Once the safe start has ended, it begins the timer by CallLater on setTimer. 
****************************************************************************************/

class ammo_Class: PS_ManualMarker 
{
	// user script
	string ammo = "{6250070685EC5A0D}Prefabs/Props/Military/AmmoBoxes/EquipmentBoxStack/USSR/FIA_ammo_box.et";
	string expl = "{6250070685EC5A0D}Prefabs/Props/Military/AmmoBoxes/EquipmentBoxStack/USSR/FIA_ammo_box.et";
	string hmg = "{6250070685EC5A0D}Prefabs/Props/Military/AmmoBoxes/EquipmentBoxStack/USSR/FIA_ammo_box.et";
	
	IEntity spawnAmmo, spawnExpl, spawnHMG;
	
	void spawnthings()
	{
		spawnAmmo = GetGame().GetWorld().FindEntityByName("ammo");
		spawnExpl = GetGame().GetWorld().FindEntityByName("explosives");
		spawnHMG = GetGame().GetWorld().FindEntityByName("hmg");

		
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		
		spawnParams.Transform[3] = spawnAmmo.GetOrigin();		
		GetGame().SpawnEntityPrefab(Resource.Load(ammo),GetGame().GetWorld(),spawnParams);
		spawnParams.Transform[3] = spawnExpl.GetOrigin();
		GetGame().SpawnEntityPrefab(Resource.Load(expl),GetGame().GetWorld(),spawnParams);
		spawnParams.Transform[3] = spawnHMG.GetOrigin();
		GetGame().SpawnEntityPrefab(Resource.Load(hmg),GetGame().GetWorld(),spawnParams);
		
		SCR_PopUpNotification.GetInstance().PopupMsg("Caches have been uncovered at their markers.", 10);
	}
	
	void safeStartCheck()
	{
		CRF_SafestartManager safestart = CRF_SafestartManager.GetInstance();
        if(safestart.GetSafestartStatus() || CRF_Gamemode.GetInstance().m_GamemodeState != CRF_EGamemodeState.GAME)
		{
         	GetGame().GetCallqueue().CallLater(safeStartCheck, 30000, false);
			return;
		}
		
		else
		{
			GetGame().GetCallqueue().CallLater(spawnthings, 60000, false);
		}
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		GetGame().GetCallqueue().CallLater(safeStartCheck, 3000, false);
	}

};


/****************************************************************************************
 * --------------Accessing a component--------------
 * Below is an example of how to access a component in an object. 
****************************************************************************************/

SCR_BaseCompartmentManagerComponent.Cast(m_eOriginalWeapon.FindComponent(SCR_BaseCompartmentManagerComponent))





/****************************************************************************************
 * --------------Object Presence Inside Control Trigger--------------
 * The script below checks for the presence of a named entity within the control 
 * area of the base trigger entity.
****************************************************************************************/

class EngiPresence_Class: SCR_BaseTriggerEntity 
{
	// user script
	bool pmessageSent = false;

	override void OnQueryFinished(bool bIsEmpty)
	{
		super.OnQueryFinished(bIsEmpty);
		
		IEntity engiTruck = GetGame().GetWorld().FindEntityByName("engiTruck");
		
		if (engiTruck)
		{
			array<IEntity> entitiesInside = new array<IEntity>();
			this.GetEntitiesInside(entitiesInside);
			foreach (IEntity entity : entitiesInside)
 	       {
				if (entity.GetName() == "engiTruck" && !pmessageSent)
				{
					SCR_PopUpNotification.GetInstance().PopupMsg("The engineering truck has made it to the tunnel", 10);
					pmessageSent = true;
				
				}
			}
		}

	}

};





/****************************************************************************************
 * --------------Action on Vehicle Destruction--------------
 * This script is attached to a vehicle, in this case an engineering truck, and
 * sends a message once the truck has been destroyed. It's utilizes the call later loop
 * from the example in section Safestart End Check & Timer to listen for the state change.
****************************************************************************************/

class engiTruck_Class: Vehicle 
{
	// user script
	bool dmessageSent = false;
	
	void checkDamage()
	{
		DamageManagerComponent dmgManager = DamageManagerComponent.Cast(this.FindComponent(DamageManagerComponent));
		
		if (dmgManager  && !dmessageSent)
		{
			if (dmgManager.IsDestroyed())
			{
				SCR_PopUpNotification.GetInstance().PopupMsg("The engineering vehicle has been destroyed", 10);
				
				dmessageSent = true;
			}
			
			else 
			{
				GetGame().GetCallqueue().CallLater(checkDamage, 3000, false);
			}
		}
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		GetGame().GetCallqueue().CallLater(checkDamage, 3000, false);

	}

};





/****************************************************************************************
 * --------------Setting an objective complete--------------
 * This is not really necessary as whoever is running the session can simply toggle 
 * objective completion, but if you want to, this is how you set it via script.
****************************************************************************************/

PS_Objective bluObjective = PS_Objective.Cast(GetGame().GetWorld().FindEntityByName("BluObj_1"));
bluObjective.SetCompleted(true);





/****************************************************************************************
 * --------------Randomizing Spawns--------------
 * This is a rough system I built for spawning a randomized weapon at one of 585 triggers. 
 * The number of triggers isn't important, other than you have to specify that number in the loop. 
 * It utilizes the same basic logic as the vehicle spawn, but with a weighted randomizer.
 * Each item to be spawned is given a range in an array and then a random
 * number generated to access that item. 
****************************************************************************************/

class weaponSpawnTrigger_Class: SCR_BaseTriggerEntity 
{
	// user script
	string g1 = "{E8F00BF730225B00}Prefabs/Weapons/Grenades/Grenade_M67.et";
	string g2 = "{645C73791ECA1698}Prefabs/Weapons/Grenades/Grenade_RGD5.et";
	string sg1 = "{9DB69176CEF0EE97}Prefabs/Weapons/Grenades/Smoke_ANM8HC.et";
	string sg2 = "{77EAE5E07DC4678A}Prefabs/Weapons/Grenades/Smoke_RDG2.et";
	string h1 = "{1353C6EAD1DCFE43}Prefabs/Weapons/Handguns/M9/Handgun_M9.et";
	string h2 = "{C0F7DD85A86B2900}Prefabs/Weapons/Handguns/PM/Handgun_PM.et";
	string l1 = "{9C5C20FB0E01E64F}Prefabs/Weapons/Launchers/M72/Launcher_M72A3.et";
	string l2 = "{7A82FE978603F137}Prefabs/Weapons/Launchers/RPG7/Launcher_RPG7.et";
	string m1 = "{D2B48DEBEF38D7D7}Prefabs/Weapons/MachineGuns/M249/MG_M249.et";
	string m2 = "{D182DCDD72BF7E34}Prefabs/Weapons/MachineGuns/M60/MG_M60.et";
	string m3 = "{A89BC9D55FFB4CD8}Prefabs/Weapons/MachineGuns/PKM/MG_PKM.et";
	string m4 = "{A7AF84C6C58BA3E8}Prefabs/Weapons/MachineGuns/RPK74/MG_RPK74.et";
	string m5 = "{026CE108BFB3EC03}Prefabs/Weapons/MachineGuns/UK59/MG_UK59.et";
	string r1 = "{FA5C25BF66A53DCF}Prefabs/Weapons/Rifles/AK74/Rifle_AK74.et";
	string r2 = "{BFEA719491610A45}Prefabs/Weapons/Rifles/AKS74U/Rifle_AKS74U.et";
	string r3 = "{B31929F65F0D0279}Prefabs/Weapons/Rifles/M14/Rifle_M21.et";
	string r4 = "{3E413771E1834D2F}Prefabs/Weapons/Rifles/M16/Rifle_M16A2.et";
	string r5 = "{3EB02CDAD5F23C82}Prefabs/Weapons/Rifles/SVD/Rifle_SVD.et";
	string r6 = "{6415B7923DE28C1B}Prefabs/Weapons/Rifles/SVD/Rifle_SVD_PSO.et";
	string r7 = "{81EB948E6414BD6F}Prefabs/Weapons/Rifles/M14/Rifle_M21_ARTII.et";
	string r8 = "{C19BFAF8A6334FD5}Prefabs/Weapons/Rifles/VZ58/Rifle_VZ58_base.et";
	string r9 = "{63E8322E2ADD4AA7}Prefabs/Weapons/Rifles/AK74/Rifle_AK74_GP25.et";
	string r10 = "{C417FBA57578B2E1}Prefabs/Weapons/Rifles/M16/Variants/Rifle_M16A2_carbine_M203_OliveGreen_Sand_Stripes.et";
	string a1 = "{0A84AA5A3884176F}Prefabs/Weapons/Magazines/Magazine_545x39_AK_30rnd_Last_5Tracer.et";
	string a2 = "{D8F2CA92583B23D3}Prefabs/Weapons/Magazines/Magazine_556x45_STANAG_30rnd_Last_5Tracer.et";
	string a3 = "{8B853CDD11BA916E}Prefabs/Weapons/Magazines/Magazine_9x18_PM_8rnd_Ball.et";
	string a4 = "{9C05543A503DB80E}Prefabs/Weapons/Magazines/Magazine_9x19_M9_15rnd_Ball.et";
	string a5 = "{9CCB46C6EE632C1A}Prefabs/Weapons/Magazines/Magazine_762x54_SVD_10rnd_Sniper.et";
	string a6 = "{627255315038152A}Prefabs/Weapons/Magazines/Magazine_762x51_M14_20rnd_SpecialBall.et";
	string a7 = "{D78C667F59829717}Prefabs/Weapons/Magazines/Magazine_545x39_RPK_45rnd_4Ball_1Tracer.et";
	string a8 = "{32E12D322E107F1C}Prefabs/Weapons/Ammo/Ammo_Rocket_PG7VM.et";
	string b1 = "{98C79F5FAE12F9B6}Prefabs/Weapons/Attachments/Bayonets/Bayonet_6Kh4.et";
	string b2 = "{558117556F3880A8}Prefabs/Weapons/Attachments/Bayonets/Bayonet_M9.et";
	string bolt = "{24FFA508861139C9}Prefabs/Weapons/Rifles/L42A1/Rifle_L8.et";
	
	ref array< ref array<int>> numberRanges = {};
	ref array<string> values = {};
	
	
	void PopulateRanges()
	{
		numberRanges.Insert({15, 29});
		values.Insert(bolt);
		numberRanges.Insert({30, 41});
		values.Insert(r1);
		numberRanges.Insert({42, 53});
		values.Insert(r2);
		numberRanges.Insert({54, 56});
		values.Insert(b1);
		numberRanges.Insert({57, 59});
		values.Insert(b2);
		numberRanges.Insert({60, 61});
		values.Insert(g1);
		numberRanges.Insert({62, 63});
		values.Insert(g2);
		numberRanges.Insert({64, 64});
		values.Insert(sg1);
		numberRanges.Insert({65, 65});
		values.Insert(sg2);
		numberRanges.Insert({66, 66});
		values.Insert(h1);
		numberRanges.Insert({67, 67});
		values.Insert(h2);
		numberRanges.Insert({68, 68});
		values.Insert(l1);
		numberRanges.Insert({69, 70});
		values.Insert(l2);
		numberRanges.Insert({71, 71});
		values.Insert(m1);
		numberRanges.Insert({72, 72});
		values.Insert(m2);
		numberRanges.Insert({73, 73});
		values.Insert(m3);
		numberRanges.Insert({74, 74});
		values.Insert(m4);
		numberRanges.Insert({75, 75});
		values.Insert(m5);
		numberRanges.Insert({76, 76});
		values.Insert(r3);
		numberRanges.Insert({77, 77});
		values.Insert(r4);
		numberRanges.Insert({78, 78});
		values.Insert(r5);
		numberRanges.Insert({79, 79});
		values.Insert(r6);
		numberRanges.Insert({80, 80});
		values.Insert(r7);
		numberRanges.Insert({81, 81});
		values.Insert(r8);
		numberRanges.Insert({82, 82});
		values.Insert(r9);
		numberRanges.Insert({83, 83});
		values.Insert(r10);
		numberRanges.Insert({84, 85});
		values.Insert(a1);
		numberRanges.Insert({86, 87});
		values.Insert(a2);
		numberRanges.Insert({88, 89});
		values.Insert(a3);
		numberRanges.Insert({90, 91});
		values.Insert(a4);
		numberRanges.Insert({92, 93});
		values.Insert(a5);
		numberRanges.Insert({94, 95});
		values.Insert(a6);
		numberRanges.Insert({96, 97});
		values.Insert(a7);
		numberRanges.Insert({98, 98});
		values.Insert(a8);
		numberRanges.Insert({99, 99});
		values.Insert(l2);
	}
	
	string GetValue() 
	{
    	int randomNumber = Math.RandomInt(0, 200); 

    	// Iterate through ranges to find a match
    	for (int i = 0; i < numberRanges.Count(); i++) 
		{
        	int start = numberRanges[i][0];
        	int end = numberRanges[i][1];
			
        	if (randomNumber >= start && randomNumber <= end) 
			{
            	return values[i]; // Return the associated value
        	}
    	}

    	return "fail";
	}
	
	void SpawnThings()
	{
		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		
		for (int i = 1; i <= 585; i++)
		{
			string childName = "weaponSpawn" + i.ToString();
			IEntity child = GetGame().GetWorld().FindEntityByName(childName);
			spawnParams.Transform[3] = child.GetOrigin();
			
			string item = GetValue();
			if (item != "fail")
			{
				GetGame().SpawnEntityPrefab(Resource.Load(item),GetGame().GetWorld(),spawnParams);
			}
			

		}
	}

	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		PopulateRanges();
		
		GetGame().GetCallqueue().CallLater(SpawnThings, 100, false);

	}

};

/****************************************************************************************
 * --------------Rush objective destructor--------------
 * A new implementation of the rush gamemode, this is the destructor for the generator
 * objective. It sends a message about the objective, finds the generator near it and 
 * destroys it, removes the appropriate area markers, and then if both objectives in 
 * the zone are destroyed respawns players at their flag.
****************************************************************************************/
class z1s1_Class: GenericEntity 
{
	void ~z1s1_Class()
	{
		SCR_PopUpNotification.GetInstance().PopupMsg("Alpha MCOM destroyed", duration: 10);
		
		// TODO: Get the parent and delete everything
		// Delete actual mcom/generator
		IEntity mcom = GetGame().GetWorld().FindEntityByName("z1s1gen");
		delete mcom;
		// Delete markers
		IEntity mcomMarker = GetGame().GetWorld().FindEntityByName("alphamarker");
		delete mcomMarker;
		
		// Check if both sites are destroyed, if so, delete the zone marker line
		IEntity sisterMcom = GetGame().GetWorld().FindEntityByName("z1s2gen");
		
		if (!sisterMcom) {
			IEntity zoneMarkers = GetGame().GetWorld().FindEntityByName("zone1markers");
			SCR_EntityHelper.DeleteEntityAndChildren(zoneMarkers);
			SCR_PopUpNotification.GetInstance().PopupMsg("Zone 2 is now unlocked", duration: 10);
			
			//respawn the players
			if (RplSession.Mode() == RplMode.Dedicated) {
				CRF_Gamemode gm = CRF_Gamemode.GetInstance();
				gm.RushRespawnPlayers();
			}
		}
	}

};

