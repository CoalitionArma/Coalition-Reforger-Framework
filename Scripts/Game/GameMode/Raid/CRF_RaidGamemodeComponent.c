class CRF_RaidGamemodeComponentClass: SCR_BaseGameModeComponentClass
{
}
 
class CRF_RaidGamemodeComponent: SCR_BaseGameModeComponent
{
	// ------------------------------------------------------------------ attrs
	[Attribute("50", UIWidgets.EditBox, "Percentage of total supply that must be destroyed to trigger an attacker victory.")]
	float m_fWinThresholdPercent;
 
	[Attribute("BLUFOR", UIWidgets.EditBox, "Faction key of the attacking side.")]
	string m_sAttackingSide;
 
	[Attribute("OPFOR", UIWidgets.EditBox, "Faction key of the defending side.")]
	string m_sDefendingSide;
 
	// ----------------------------------------------------------------- state
	protected static CRF_RaidGamemodeComponent m_sInstance;
 
	// Registered items — populated during EOnInit across all RaidItemComponents
	protected ref array<CRF_RaidItemComponent> m_aRegisteredItems = {};
 
	// Supply totals — m_iTotalSupply is finalized on the first destruction event
	protected int  m_iTotalSupply		= 0;
	protected int  m_iDestroyedSupply	= 0;
	protected bool m_bTotalsReady		= false;
	protected bool m_bVictoryTriggered	= false;
 
	// Client-side HUD state
	protected Widget m_wCurrentAlert;
	protected int    m_iWidgetGeneration = 0;   // used to cancel stale fade callbacks
 
	// --------------------------------------------------------------- ctor/dtor
	void CRF_RaidGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_sInstance = this;
	}
 
	void ~CRF_RaidGamemodeComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		if (!GetGame() || !GetGame().GetWorld())
			return;
 
		if (m_wCurrentAlert)
			delete m_wCurrentAlert;
	}
 
	// ---------------------------------------------------------------- static
	static CRF_RaidGamemodeComponent GetInstance()
	{
		return m_sInstance;
	}
 
	// --------------------------------------------------------------- init
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
 
		#ifndef WORKBENCH
		if (!System.IsConsoleApp())
			return;
		#endif
 
		SetEventMask(owner, EntityEvent.INIT);
	}
 
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		// Supply total is finalized lazily on the first destruction event,
		// so there is nothing to schedule here.
	}
 
	// ---------------------------------------------------------- registration api
	// Called by each CRF_RaidItemComponent during its own EOnInit.
	void RegisterRaidItem(CRF_RaidItemComponent item)
	{
		if (!item)
			return;
 
		m_aRegisteredItems.Insert(item);
	}
 
	// ---------------------------------------------------------- supply totalling
	// Called once, on the first destruction event, to lock in the total.
	// By the time a player can destroy anything the scenario is fully loaded,
	// so every item will have already called RegisterRaidItem().
	protected void FinalizeSupplyTotal()
	{
		m_iTotalSupply = 0;
		foreach (CRF_RaidItemComponent item : m_aRegisteredItems)
		{
			if (item)
				m_iTotalSupply += item.GetSupplyValue();
		}
 
		m_bTotalsReady = true;
		Print(string.Format("[CRF_Raid] Supply pool finalized on first destruction: %1 total supply across %2 registered items.",
			m_iTotalSupply, m_aRegisteredItems.Count()));
	}
 
	// ----------------------------------------------------------- destruction api
	// Called by CRF_RaidItemComponent when its entity reaches EDamageState.DESTROYED.
	void OnItemDestroyed(CRF_RaidItemComponent item)
	{
		if (!item || m_bVictoryTriggered)
			return;
 
		// Finalize the supply total on the very first destruction if not yet done.
		// This is more reliable than a fixed startup delay on slow-loading servers.
		if (!m_bTotalsReady)
			FinalizeSupplyTotal();
 
		m_iDestroyedSupply += item.GetSupplyValue();
 
		if (m_iTotalSupply == 0)
		{
			Print("[CRF_Raid] WARNING: Total supply is 0 after finalization — check that RaidItemComponents registered correctly.", LogLevel.WARNING);
			return;
		}
 
		float destroyedPercent = (float)m_iDestroyedSupply / (float)m_iTotalSupply * 100.0;
 
		// Broadcast HUD update to all clients
		Rpc(RpcDo_ShowDestructionUpdate, destroyedPercent);
 
		// Check win condition
		if (destroyedPercent >= m_fWinThresholdPercent && !m_bVictoryTriggered)
		{
			m_bVictoryTriggered = true;
			Rpc(RpcDo_BroadcastMessage, string.Format("Attackers have destroyed enough equipment! %1%% destroyed — attacker victory!",
				Math.Round(destroyedPercent).ToString()));
		}
	}
 
	// ---------------------------------------------------------------- RPCs
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_BroadcastMessage(string message)
	{
		SCR_PopUpNotification.GetInstance().PopupMsg(message);
	}
 
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_ShowDestructionUpdate(float destroyedPercent)
	{
		// Clean up any existing alert widget
		if (m_wCurrentAlert)
		{
			AnimateWidget.StopAllAnimations(m_wCurrentAlert);
			delete m_wCurrentAlert;
		}
 
		m_iWidgetGeneration++;
		int thisGeneration = m_iWidgetGeneration;
 
		m_wCurrentAlert = GetGame().GetWorkspace().CreateWidgets("{66DCB94B8F932419}UI/layouts/HUD/Raid/RaidPopUp.layout");
		if (!m_wCurrentAlert)
			return;
 
		m_wCurrentAlert.SetOpacity(0);
		AnimateWidget.Opacity(m_wCurrentAlert, 1, 0.5);
 
		// "X% / Y%" label
		TextWidget labelWidget = TextWidget.Cast(m_wCurrentAlert.FindWidget("TotalAdded"));
		if (labelWidget)
			labelWidget.SetText(string.Format("%1%% / %2%%",
				Math.Round(destroyedPercent).ToString(),
				Math.Round(m_fWinThresholdPercent).ToString()));
 
		// Progress bar scaled so 100% full = win threshold reached
		ProgressBarWidget progressWidget = ProgressBarWidget.Cast(m_wCurrentAlert.FindWidget("Progress"));
		if (progressWidget)
		{
			float barPercent = destroyedPercent / m_fWinThresholdPercent * 100.0;
			progressWidget.SetCurrent(Math.Clamp(barPercent, 0, 100));
		}
 
		// Fade out after 5 seconds, but only if no newer alert has appeared
		GetGame().GetCallqueue().CallLater(FadeAlert, 5000, false, thisGeneration);
	}
 
	// --------------------------------------------------------------- helpers
	protected void FadeAlert(int generation)
	{
		// A newer alert has since been shown — let that one manage its own fade
		if (generation != m_iWidgetGeneration || !m_wCurrentAlert)
			return;
 
		AnimateWidget.Opacity(m_wCurrentAlert, 0, 1.0);
		GetGame().GetCallqueue().CallLater(DeleteAlert, 1100, false, generation);
	}
 
	protected void DeleteAlert(int generation)
	{
		if (generation != m_iWidgetGeneration || !m_wCurrentAlert)
			return;
 
		delete m_wCurrentAlert;
		m_wCurrentAlert = null;
	}
}