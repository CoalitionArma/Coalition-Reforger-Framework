class CRF_SpectatorDamageReportEntry
{
	int m_iVictimPlayerId;
	string m_sVictimName;
	int m_iAttackerPlayerId;
	string m_sAttackerName;
	float m_fDamageValue;
	float m_fRangeMeters;
	string m_sDamageType;
	string m_sHitZone;
	string m_sBodyRegion;
	bool m_bFatal;
	int m_iWorldTime;

	void CRF_SpectatorDamageReportEntry(int victimPlayerId, string victimName, int attackerPlayerId, string attackerName, float damageValue, float rangeMeters, string damageType, string hitZone, string bodyRegion, bool fatal, int worldTime)
	{
		m_iVictimPlayerId = victimPlayerId;
		m_sVictimName = victimName;
		m_iAttackerPlayerId = attackerPlayerId;
		m_sAttackerName = attackerName;
		m_fDamageValue = damageValue;
		m_fRangeMeters = rangeMeters;
		m_sDamageType = damageType;
		m_sHitZone = hitZone;
		m_sBodyRegion = bodyRegion;
		m_bFatal = fatal;
		m_iWorldTime = worldTime;
	}
}

class CRF_SpectatorDamageReportStore
{
	protected const int MAX_DAMAGE_EVENTS = 500;
	protected static ref array<ref CRF_SpectatorDamageReportEntry> s_aEntries = new array<ref CRF_SpectatorDamageReportEntry>();

	static void InsertEvent(int victimPlayerId, string victimName, int attackerPlayerId, string attackerName, float damageValue, float rangeMeters, string damageType, string hitZone, string bodyRegion, bool fatal, int worldTime)
	{
		if (victimPlayerId <= 0)
			return;

		if (victimName.IsEmpty())
			victimName = "Unknown";

		if (attackerName.IsEmpty())
			attackerName = "Environment";

		if (damageType.IsEmpty())
			damageType = "Unknown";

		if (hitZone.IsEmpty())
			hitZone = "Unknown";

		if (bodyRegion.IsEmpty())
			bodyRegion = "Unknown";

		s_aEntries.Insert(new CRF_SpectatorDamageReportEntry(victimPlayerId, victimName, attackerPlayerId, attackerName, damageValue, rangeMeters, damageType, hitZone, bodyRegion, fatal, worldTime));

		while (s_aEntries.Count() > MAX_DAMAGE_EVENTS)
			s_aEntries.RemoveOrdered(0);
	}

	static void GetEventsForPlayer(int playerId, notnull array<ref CRF_SpectatorDamageReportEntry> outEntries)
	{
		outEntries.Clear();

		if (playerId <= 0)
			return;

		foreach (CRF_SpectatorDamageReportEntry entry : s_aEntries)
		{
			if (entry.m_iVictimPlayerId == playerId || entry.m_iAttackerPlayerId == playerId)
				outEntries.Insert(entry);
		}
	}

	static string GetFatalBodyRegion(int playerId)
	{
		for (int i = s_aEntries.Count() - 1; i >= 0; i--)
		{
			CRF_SpectatorDamageReportEntry entry = s_aEntries[i];
			if (entry.m_iVictimPlayerId == playerId && entry.m_bFatal)
				return entry.m_sBodyRegion;
		}

		return string.Empty;
	}

	static void MarkLatestVictimEventFatal(int playerId)
	{
		if (playerId <= 0)
			return;

		for (int i = s_aEntries.Count() - 1; i >= 0; i--)
		{
			CRF_SpectatorDamageReportEntry entry = s_aEntries[i];
			if (entry.m_iVictimPlayerId != playerId)
				continue;

			entry.m_bFatal = true;
			return;
		}
	}
}

modded class SCR_DamageManagerComponent
{
	override protected void OnDamage(notnull BaseDamageContext damageContext)
	{
		IEntity victimEntity = GetOwner();
		PlayerManager playerManager = GetGame().GetPlayerManager();
		int victimPlayerId = 0;
		bool wasAlive = false;
		bool shouldReport = CRF_ShouldReportSpectatorDamage(damageContext);

		if (victimEntity && playerManager)
		{
			victimPlayerId = playerManager.GetPlayerIdFromControlledEntity(victimEntity);
			if (victimPlayerId > 0)
				wasAlive = CRF_DamageHelper.CheckIfEntityAlive(victimEntity);
		}

		super.OnDamage(damageContext);

		if (victimPlayerId <= 0 || damageContext.damageValue <= 0)
			return;

		if (!shouldReport)
			return;

		#ifndef WORKBENCH
		if (RplSession.Mode() == RplMode.Client)
			return;
		#endif

		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (!broadcastManager)
			return;

		int attackerPlayerId = 0;
		if (damageContext.instigator)
			attackerPlayerId = damageContext.instigator.GetInstigatorPlayerID();

		string victimName = playerManager.GetPlayerName(victimPlayerId);
		string attackerName = "Environment";
		if (attackerPlayerId > 0)
			attackerName = playerManager.GetPlayerName(attackerPlayerId);

		string hitZoneName = GetDamageReportHitZoneName(damageContext);
		string bodyRegion = GetDamageReportBodyRegion(victimEntity, damageContext, hitZoneName);
		string damageType = CRF_DamageHelper.GetDamageTypeString(damageContext.damageType);
		bool fatal = wasAlive && !CRF_DamageHelper.CheckIfEntityAlive(victimEntity);
		int worldTime = GetGame().GetWorld().GetWorldTime();
		float rangeMeters = GetDamageReportRangeMeters(victimEntity, damageContext, attackerPlayerId);

		broadcastManager.BroadcastSpectatorDamageReport(victimPlayerId, victimName, attackerPlayerId, attackerName, damageContext.damageValue, rangeMeters, damageType, hitZoneName, bodyRegion, fatal, worldTime);
	}

	protected void CRF_MarkLatestSpectatorDamageReportFatal()
	{
		IEntity victimEntity = GetOwner();
		if (!victimEntity)
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return;

		int victimPlayerId = playerManager.GetPlayerIdFromControlledEntity(victimEntity);
		if (victimPlayerId <= 0)
			return;

		CRF_RplBroadcastManager broadcastManager = CRF_RplBroadcastManager.GetInstance();
		if (broadcastManager)
			broadcastManager.BroadcastSpectatorDamageReportFatal(victimPlayerId);
	}

	protected bool CRF_ShouldReportSpectatorDamage(notnull BaseDamageContext damageContext)
	{
		if (!damageContext.struckHitZone)
			return true;

		SCR_CharacterDamageManagerComponent characterDamageManager = SCR_CharacterDamageManagerComponent.Cast(this);
		if (!characterDamageManager)
			return true;

		array<HitZone> physicalHitZones = {};
		GetPhysicalHitZones(physicalHitZones);
		if (physicalHitZones.IsEmpty())
			return true;

		foreach (HitZone physicalHitZone : physicalHitZones)
		{
			if (physicalHitZone == damageContext.struckHitZone)
				return true;
		}

		return false;
	}

	protected float GetDamageReportRangeMeters(IEntity victimEntity, notnull BaseDamageContext damageContext, int attackerPlayerId)
	{
		if (!victimEntity)
			return -1;

		IEntity attackerEntity;
		if (damageContext.instigator)
		{
			attackerEntity = damageContext.instigator.GetInstigatorEntity();
			if (!attackerEntity && attackerPlayerId > 0)
				attackerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(attackerPlayerId);
		}

		if (!attackerEntity)
			return -1;

		return vector.Distance(victimEntity.GetOrigin(), attackerEntity.GetOrigin());
	}

	protected string GetDamageReportHitZoneName(notnull BaseDamageContext damageContext)
	{
		if (!damageContext.struckHitZone)
			return "Unknown";

		string hitZoneName = damageContext.struckHitZone.GetName();
		if (hitZoneName.IsEmpty())
			return "Unknown";

		return hitZoneName;
	}

	protected string GetDamageReportBodyRegion(IEntity victimEntity, notnull BaseDamageContext damageContext, string hitZoneName)
	{
		string lowerHitZone = hitZoneName;
		lowerHitZone.ToLower();

		if (lowerHitZone.Contains("head") || lowerHitZone.Contains("neck"))
			return "Head";

		if (lowerHitZone.Contains("arm") || lowerHitZone.Contains("hand") || lowerHitZone.Contains("shoulder"))
		{
			if (lowerHitZone.Contains("left") || lowerHitZone.Contains("l_"))
				return "Left Arm";

			if (lowerHitZone.Contains("right") || lowerHitZone.Contains("r_"))
				return "Right Arm";

			return "Arm";
		}

		if (lowerHitZone.Contains("leg") || lowerHitZone.Contains("foot") || lowerHitZone.Contains("thigh") || lowerHitZone.Contains("calf"))
		{
			if (lowerHitZone.Contains("left") || lowerHitZone.Contains("l_"))
				return "Left Leg";

			if (lowerHitZone.Contains("right") || lowerHitZone.Contains("r_"))
				return "Right Leg";

			return "Leg";
		}

		if (lowerHitZone.Contains("torso") || lowerHitZone.Contains("spine") || lowerHitZone.Contains("chest") || lowerHitZone.Contains("abdomen") || lowerHitZone.Contains("pelvis") || lowerHitZone.Contains("body"))
			return "Torso";

		if (victimEntity)
		{
			vector localHit = victimEntity.CoordToLocal(damageContext.hitPosition);
			if (localHit[1] > 1.45)
				return "Head";

			if (localHit[1] > 0.85)
				return "Torso";

			return "Leg";
		}

		return "Unknown";
	}
}
