class CRF_MagazineCheckComponentClass : ScriptComponentClass {}

class CRF_MagazineCheckComponent : ScriptComponent
{
	// How long to hold the vanilla weapon-inspect pose before showing the hint.
	protected const int INSPECT_DURATION_MS = 1500;

	protected InputManager m_InputManager;
	protected SCR_PlayerController m_PlayerController;
	protected SCR_CharacterControllerComponent m_InspectingController;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		if (SCR_Global.IsEditMode())
			return;
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		Print("[CRF_MagazineCheck] EOnInit called", LogLevel.NORMAL);

		if (SCR_Global.IsEditMode())
		{
			Print("[CRF_MagazineCheck] Aborting: IsEditMode", LogLevel.NORMAL);
			return;
		}

		m_PlayerController = SCR_PlayerController.Cast(owner);
		if (!m_PlayerController)
		{
			Print("[CRF_MagazineCheck] Aborting: owner is not SCR_PlayerController", LogLevel.NORMAL);
			return;
		}

		if (m_PlayerController.GetPlayerId() != SCR_PlayerController.GetLocalPlayerId())
		{
			Print(string.Format("[CRF_MagazineCheck] Aborting: not local player (owner=%1, local=%2)", m_PlayerController.GetPlayerId(), SCR_PlayerController.GetLocalPlayerId()), LogLevel.NORMAL);
			return;
		}

		m_InputManager = GetGame().GetInputManager();
		if (m_InputManager)
		{
			m_InputManager.AddActionListener("CRF_CheckMagazine", EActionTrigger.DOWN, OnCheckMagazinePressed);
			Print("[CRF_MagazineCheck] Action listener registered for CRF_CheckMagazine", LogLevel.NORMAL);
		}
		else
		{
			Print("[CRF_MagazineCheck] Aborting: no InputManager", LogLevel.NORMAL);
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (m_InputManager)
			m_InputManager.RemoveActionListener("CRF_CheckMagazine", EActionTrigger.DOWN, OnCheckMagazinePressed);

		GetGame().GetCallqueue().Remove(EndInspectionAndShowHint);
		if (m_InspectingController && m_InspectingController.GetInspect())
			m_InspectingController.SetInspect(null);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCheckMagazinePressed()
	{
		Print("[CRF_MagazineCheck] >>> Key combo (Ctrl+R) pressed - CRF_CheckMagazine action triggered <<<", LogLevel.WARNING);

		// Avoid stacking multiple delayed hints/animations if the player mashes the combo.
		GetGame().GetCallqueue().Remove(EndInspectionAndShowHint);

		if (!m_PlayerController)
			return;

		IEntity character = m_PlayerController.GetControlledEntity();
		if (!character)
			return;

		SCR_CharacterControllerComponent charController = SCR_CharacterControllerComponent.Cast(character.FindComponent(SCR_CharacterControllerComponent));
		if (!charController)
			return;

		BaseWeaponManagerComponent weaponManager = charController.GetWeaponManagerComponent();
		if (!weaponManager)
			return;

		BaseWeaponComponent weapon = weaponManager.GetCurrentWeapon();
		if (!weapon)
		{
			SCR_HintManagerComponent.ShowCustomHint("No weapon equipped", "Magazine Check", 3);
			return;
		}

		// Reuse the same vanilla weapon-inspect pose that plays when holding reload,
		// instead of authoring a new animation.
		IEntity weaponEntity = weapon.GetOwner();
		if (weaponEntity && charController.CanInspect(weaponEntity))
		{
			charController.SetInspect(weaponEntity);
			m_InspectingController = charController;
			GetGame().GetCallqueue().CallLater(EndInspectionAndShowHint, INSPECT_DURATION_MS, false);
			return;
		}

		// Couldn't enter inspect (e.g. in a vehicle) - just show the hint immediately.
		ShowMagazineHint(charController);
	}

	//------------------------------------------------------------------------------------------------
	protected void EndInspectionAndShowHint()
	{
		SCR_CharacterControllerComponent charController = m_InspectingController;
		m_InspectingController = null;

		if (charController && charController.GetInspect())
			charController.SetInspect(null);

		ShowMagazineHint(charController);
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowMagazineHint(SCR_CharacterControllerComponent charController)
	{
		if (!charController)
			return;

		BaseWeaponManagerComponent weaponManager = charController.GetWeaponManagerComponent();
		if (!weaponManager)
			return;

		BaseWeaponComponent weapon = weaponManager.GetCurrentWeapon();
		if (!weapon)
		{
			SCR_HintManagerComponent.ShowCustomHint("No weapon equipped", "Magazine Check", 3);
			return;
		}

		// Query via the currently selected muzzle so multi-barrel weapons (e.g. underslung
		// grenade launchers) report the magazine for whatever fire mode is actually active.
		BaseMagazineComponent magazine;
		BaseMuzzleComponent muzzle = weapon.GetCurrentMuzzle();
		if (muzzle)
			magazine = muzzle.GetMagazine();
		if (!magazine)
			magazine = weapon.GetCurrentMagazine();

		if (!magazine)
		{
			SCR_HintManagerComponent.ShowCustomHint("Empty", "Magazine Check", 3);
			return;
		}

		SCR_HintManagerComponent.ShowCustomHint(GetMagazineStatusText(magazine.GetAmmoCount(), magazine.GetMaxAmmoCount()), "Magazine Check", 3);
	}

	//------------------------------------------------------------------------------------------------
	protected string GetMagazineStatusText(int current, int max)
	{
		if (max <= 0 || current <= 0)
			return "Empty";

		float ratio = current / (float)max;

		if (ratio >= 0.99)
			return "Full";
		if (ratio >= 0.75)
			return "Nearly full";
		if (ratio >= 0.5)
			return "Half full";
		if (ratio >= 0.25)
			return "1/4 full";

		return "Empty";
	}
}
