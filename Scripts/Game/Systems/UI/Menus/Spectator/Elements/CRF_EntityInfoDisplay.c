class CRF_EntityInfoDisplay : SCR_ScriptedWidgetComponent
{		
	protected Widget        m_wEntityInfoDisplay;
	protected TextWidget    m_wEntityName;
	protected TextWidget    m_wEntityRole;
	protected TextWidget    m_wEntityDamage;  // ACE blood state label (left side of status row)
	protected TextWidget    m_wEntityDamageType;     // "BLEEDING" indicator (right side of status row)
	protected SliderWidget  m_wEntityHealthSlider;
	
	protected IEntity m_eSpecEntity;
	
	//------------------------------------------------------------------------------------------------
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		m_wEntityInfoDisplay = w;
		m_wEntityName = TextWidget.Cast(w.FindAnyWidget("EntityName"));
		m_wEntityRole = TextWidget.Cast(w.FindAnyWidget("EntityRole"));
		m_wEntityDamage = TextWidget.Cast(w.FindAnyWidget("EntityDamage"));
		m_wEntityDamageType = TextWidget.Cast(w.FindAnyWidget("EntityDamageType"));
		m_wEntityHealthSlider = SliderWidget.Cast(w.FindAnyWidget("EntityHealthSlider"));
	}

	/**
	 * Updates the follow-mode HUD overlay shown when latched onto a player in TPP or FPP mode.
	 * Displays: player name, role name, faction-colored health bar.
	 * Hidden automatically when not following anyone.
	 */
	void UpdateEntityInfoDisplay(IEntity m_eSpecEntity)
	{
		if (!m_wEntityInfoDisplay)
			return;

		// Hide the HUD if we are not following anyone
		if (!m_eSpecEntity)
		{
			m_wEntityInfoDisplay.SetVisible(false);
			return;
		}

		m_wEntityInfoDisplay.SetVisible(true);

		string playerName = "";
		RplComponent rpl = RplComponent.Cast(m_eSpecEntity.FindComponent(RplComponent));
		if (rpl)
		{
			CRF_SlotDataContainer slotData = CRF_SlottingManager.GetInstance().GetSlotDataFromCharacter(rpl.Id());
			int playerId = 0;
			if (slotData)
				playerId = slotData.GetSlotCurrentPlayerId();
			if (playerId > 0)
				playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
			if (playerName.IsEmpty() && slotData)
				playerName = slotData.GetSlotName();
		}

		// Truncate with ellipsis if the name is too long (~36 chars at bold font 18)
		const int NAME_MAX_CHARS = 36;
		if (playerName.Length() > NAME_MAX_CHARS)
			playerName = playerName.Substring(0, NAME_MAX_CHARS - 1) + "…";

		m_wEntityName.SetText(playerName);

		// --- Role name ---
		string roleName = "";
		if (rpl)
		{
			CRF_SlotDataContainer slotData = CRF_SlottingManager.GetInstance().GetSlotDataFromCharacter(rpl.Id());
			if (slotData)
			{
				roleName = slotData.GetSlotName();

				// Prepend group name if available
				int rolePlayerId = slotData.GetSlotCurrentPlayerId();
				if (rolePlayerId > 0)
				{
					SCR_AIGroup playerGroup = CRF_SlottingManager.GetInstance().GetPlayerSlotGroup(rolePlayerId);
					if (playerGroup)
					{
						string groupName = playerGroup.GetCustomName();
						if (groupName.IsEmpty())
							groupName = playerGroup.GetCustomNameWithOriginal();
						if (!groupName.IsEmpty())
							roleName = groupName + " | " + roleName;
					}
				}
			}
		}

		// Append vehicle info if the character is inside one
		IEntity vehicle = SCR_CompartmentAccessComponent.GetVehicleIn(m_eSpecEntity);
		if (vehicle)
		{
			SCR_EditableVehicleComponent editableVehicle = SCR_EditableVehicleComponent.Cast(vehicle.FindComponent(SCR_EditableVehicleComponent));
			string vehicleName = "";
			if (editableVehicle)
			{
				SCR_UIInfo vehicleInfo = editableVehicle.GetInfo();
				if (vehicleInfo)
					vehicleName = vehicleInfo.GetName();
			}
			if (!vehicleName.IsEmpty())
				roleName = roleName + "  |  " + vehicleName;
			else
				roleName = roleName + "  |  In Vehicle";
		}

		// Truncate with ellipsis if the string is too long to fit the role line (~48 chars at font 14)
		const int ROLE_MAX_CHARS = 48;
		if (roleName.Length() > ROLE_MAX_CHARS)
			roleName = roleName.Substring(0, ROLE_MAX_CHARS - 1) + "…";

		m_wEntityRole.SetText(roleName);

		// --- ACE Medical status (blood volume bar + blood state + bleeding indicator) ---
		SCR_CharacterDamageManagerComponent charDmg = SCR_CharacterDamageManagerComponent.Cast(
			m_eSpecEntity.FindComponent(SCR_CharacterDamageManagerComponent));

		// Blood volume: prefer ACE blood hit zone (tracks bleeding), fall back to vanilla health
		float bloodScaled = 1.0;
		bool isBleeding = false;
		bool isDead = false;
		bool isUnconscious = false;
		string damageStateText = "";
		string bloodStateText = "";
		if (charDmg)
		{
			SCR_CharacterBloodHitZone bloodHZ = SCR_CharacterBloodHitZone.Cast(charDmg.GetBloodHitZone());
			if (bloodHZ)
			{
				bloodScaled = bloodHZ.GetHealthScaled();

				// Map blood damage state to a readable label.
				// ECharacterBloodState integer values per ACE-Anvil:
				//   0 = NORMAL, 1 = CLASS_1_HEMORRHAGE, 3 = CLASS_2_HEMORRHAGE,
				//   4 = CLASS_3_HEMORRHAGE, 5 = CLASS_4_HEMORRHAGE,
				//   2 = FATAL, UNCONSCIOUS maps to vanilla EDamageState.
				int bloodState = bloodHZ.GetDamageState();
				if (bloodState == 0)
					bloodStateText = "Healthy";
				else if (bloodState == 1)
					bloodStateText = "Class I Hemorrhage";
				else if (bloodState == 3)
					bloodStateText = "Class II Hemorrhage";
				else if (bloodState == 4)
					bloodStateText = "Class III Hemorrhage";
				else if (bloodState == 5)
					bloodStateText = "Class IV Hemorrhage";
				else if (bloodState == 2)
					bloodStateText = "Fatal Blood Loss";
				else
					bloodStateText = "Hemorrhagic Shock";
			}
			else
			{
				// ACE bleeding not available — fall back to vanilla health
				bloodScaled = charDmg.GetHealthScaled();
				bloodStateText = "Healthy";
			}
			
			isDead = !CRF_DamageUtility.CheckIfEntityAlive(m_eSpecEntity);
			
			if (isDead)
			{
				bloodScaled = 0;
				
				BaseDamageEffect fatalDamageEffect = charDmg.GetFatalDamageEffect();
				
				if (fatalDamageEffect)
				{
					damageStateText = CRF_DamageUtility.GetCauseOfDeathString(fatalDamageEffect.GetDamageType());
					
					// Instigator can be null for environmental kills (falls, fire, etc.) — guard before chaining
					Instigator instigator = fatalDamageEffect.GetInstigator();
					if (instigator)
					{
						int killerPlayerId = instigator.GetInstigatorPlayerID();
						string killerName = GetGame().GetPlayerManager().GetPlayerName(killerPlayerId);
						
						// GetPlayerName returns empty string for AI / non-player instigators
						if (killerName.IsEmpty())
							bloodStateText = "KIA - Killed By: AI";
						else
							bloodStateText = string.Format("KIA - Killed By: %1", killerName);
					}
					else
					{
						bloodStateText = "KIA";
					};
				};
			} else
				isBleeding = charDmg.IsBleeding();
		}
		
		// --- Blood state label ---
		m_wEntityDamage.SetText(bloodStateText);

		// --- Blood bar fill ---
		// Drive the fill purely by anchors: AnchorMin.x = 0, AnchorMax.x = bloodScaled.
		// This is resolution/DPI-independent — no pixel math required.
		float clamped = Math.Clamp(bloodScaled, 0.0, 1.0);
		m_wEntityHealthSlider.SetCurrent(clamped);

		// Color: green → yellow → red as blood drops; deep red when critically low
		Color barColor;
		if (bloodScaled > 0.75)
			barColor = Color.FromRGBA(25, 191, 25, 255);    // green  — normal
		else if (bloodScaled > 0.5)
			barColor = Color.FromRGBA(220, 180, 20, 255);   // yellow — Class I/II
		else if (bloodScaled > 0.25)
			barColor = Color.FromRGBA(210, 100, 20, 255);   // orange — Class III
		else if (bloodScaled > 0)
			barColor = Color.FromRGBA(200, 30, 30, 255);    // red    — Class IV / fatal
		else
			barColor = Color.FromRGBA(80, 80, 80, 255);	 // grey   — Dead

		m_wEntityHealthSlider.SetColor(barColor);
		
		// --- Bleeding / unconscious indicator ---
		SCR_CharacterControllerComponent ctrl = SCR_CharacterControllerComponent.Cast(
			m_eSpecEntity.FindComponent(SCR_CharacterControllerComponent));

		// Check if the spectated character is unconscious and get their resilience %
		if (ctrl && ctrl.IsUnconscious() && !isDead)
		{
			isUnconscious = true;
			if (charDmg)
			{
				SCR_CharacterResilienceHitZone resHz = SCR_CharacterResilienceHitZone.Cast(
					charDmg.GetResilienceHitZone());
				if (resHz)
					damageStateText = "UNCON " + Math.Round(resHz.GetHealthScaled() * 100) + "%";
				else
					damageStateText = "UNCON";
			}
			else
				damageStateText = "UNCON";
		}
		
		if (isDead)
		{
			m_wEntityDamageType.SetText(damageStateText);
			m_wEntityDamageType.SetColor(new Color(0.5, 0.5, 0.5, 1.0));
			return;
		}
		else if (isUnconscious && isBleeding)
		{
			// Both — combine into one label, colour orange (bleeding is already implied as critical)
			m_wEntityDamageType.SetText(damageStateText + " | BLEEDING");
			m_wEntityDamageType.SetColor(new Color(1.0, 0.5, 0.0, 1.0));
		}
		else if (isUnconscious)
		{
			m_wEntityDamageType.SetText(damageStateText);
			m_wEntityDamageType.SetColor(new Color(1.0, 0.5, 0.0, 1.0));
		}
		else if (isBleeding)
		{
			m_wEntityDamageType.SetText("BLEEDING");
			m_wEntityDamageType.SetColor(new Color(0.9, 0.15, 0.15, 1));
		}
		else
		{
			m_wEntityDamageType.SetText("");
		}
	}
};