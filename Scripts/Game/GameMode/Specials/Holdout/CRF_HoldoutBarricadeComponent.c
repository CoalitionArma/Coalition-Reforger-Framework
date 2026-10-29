//------------------------------------------------------------------------------------
// CRF_HoldoutBarricadeComponent: a boarded-up doorway or window for the Holdout mode
// (CRF_HoldoutGamemodeComponent)
//
// The entity sits at the bottom of the opening, facing out. Its boards (m_iBoards of GetMaxBoards())
// are replicated as a count; every machine builds the board props itself from it (local entities with
// collision, so they block zombies and players alike), which keeps a full barricade to one replicated int.
//
// Server: zombies within m_fZombieReach wear the top board down at m_fDamagePerZombie a second each;
// at m_fBoardHealth it comes off. Players with an entrenching tool put boards back
// (CRF_HoldoutRepairBarricadeAction), and are paid for it. A window with no boards left is climbed
// through: zombies at it outside are moved in, one a second, to the point inside it. A bare doorway
// is walked through, but the AI doesn't reliably path through a doorway whose door was removed (it
// stalls on the threshold, and the Bacon AI hops as it tries to climb), so zombies standing still
// outside one are moved in the same way, two a second.
//
// The Holdout mode spawns these at a building's ground-floor openings and calls Setup with each one's
// size; placed by hand, the default size is used.
//------------------------------------------------------------------------------------

class CRF_HoldoutBarricadeComponentClass : ScriptComponentClass {}

class CRF_HoldoutBarricadeComponent : ScriptComponent
{
	[Attribute("3", UIWidgets.EditBox, "Boards in a full barricade (one row of panels each). The Holdout upgrade adds one.", params: "1 6 1")]
	protected int m_iMaxBoards;

	[Attribute("100", UIWidgets.EditBox, "Damage a board takes before it comes off (the Holdout upgrade raises it)", params: "1 10000 1")]
	protected float m_fBoardHealth;

	[Attribute("12", UIWidgets.EditBox, "Damage a second each zombie in reach does to the top board", params: "0 1000 0.5")]
	protected float m_fDamagePerZombie;

	[Attribute("2.5", UIWidgets.EditBox, "Metres from the barricade a zombie tears at it (or climbs through it) from", params: "0.5 10 0.1")]
	protected float m_fZombieReach;

	[Attribute("1.2", UIWidgets.EditBox, "Width of the opening (metres) when placed by hand", params: "0.5 6 0.05")]
	protected float m_fDefaultWidth;

	[Attribute("2.0", UIWidgets.EditBox, "Height of the opening (metres) when placed by hand", params: "0.5 4 0.05")]
	protected float m_fDefaultHeight;

	[Attribute("{7C1AEA0000000102}Prefabs/Props/Holdout/CRF_HoldoutBoard_Pallet.et", UIWidgets.ResourceNamePicker, "Board prop: a panel about 1.2 m wide, lying flat in its own frame (it is stood up across the opening)", params: "et")]
	protected ResourceName m_sBoardPrefab;

	[Attribute("BACON_622120A5448725E3_FACTION", UIWidgets.EditBox, "Faction of the zombies that tear boards off")]
	protected string m_sZombieFactionKey;

	[RplProp(onRplName: "OnBoardsChanged")]
	protected int m_iBoards;

	[RplProp(onRplName: "OnBoardsChanged")]
	protected float m_fWidth;

	[RplProp(onRplName: "OnBoardsChanged")]
	protected float m_fHeight;

	[RplProp()]
	protected bool m_bWindow;

	// Server: the floor inside a window, where climbing zombies land
	protected vector m_vInside;

	// Server: damage on the top board so far, and the zombies found in reach by the last query
	protected float m_fTopBoardDamage;
	protected ref array<IEntity> m_aZombiesInReach = {};

	// Every machine: the board props it built
	protected ref array<IEntity> m_aBoardProps = {};
	protected int m_iBuiltBoards = -1;
	protected float m_fBuiltWidth = -1;
	protected float m_fBuiltHeight = -1;

	protected ref array<IEntity> m_aDoorsFound = {};

	protected static ref array<CRF_HoldoutBarricadeComponent> s_aAll = {};

	// One panel's width; wider openings get panels side by side
	protected static const float PANEL_WIDTH = 1.15;

	// Zombies a bare opening lets in a second: a window is climbed one at a time
	protected static const int WINDOW_PASS_PER_TICK = 1;
	protected static const int DOOR_PASS_PER_TICK = 2;

	// A zombie moving slower than this (m/s, across the ground) is stuck, not walking
	protected static const float STALLED_SPEED = 0.5;

	//------------------------------------------------------------------------------------------------
	//! Every barricade on this machine
	static array<CRF_HoldoutBarricadeComponent> GetAll()
	{
		return s_aAll;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!GetGame().InPlayMode())
			return;

		s_aAll.Insert(this);
		if (m_fWidth <= 0)
			m_fWidth = m_fDefaultWidth;
		if (m_fHeight <= 0)
			m_fHeight = m_fDefaultHeight;

		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// A door that hung here would block the boards (and swing through them)
		RemoveDoors();
		RebuildBoards();

		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(ServerTick, 1000, true);
		else
			GetGame().GetCallqueue().CallLater(RebuildBoards, 1000, true); // see OnBoardsChanged
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		s_aAll.RemoveItem(this);

		if (GetGame())
		{
			GetGame().GetCallqueue().Remove(ServerTick);
			GetGame().GetCallqueue().Remove(RebuildBoards);
		}

		ClearBoards();
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Whether a character is standing still or stuck (hopping in place counts: only movement across
	//! the ground is measured)
	static bool IsStalled(notnull IEntity character)
	{
		Physics physics = character.GetPhysics();
		if (!physics)
			return true;

		vector velocity = physics.GetVelocity();
		velocity[1] = 0;
		return velocity.Length() < STALLED_SPEED;
	}

	//------------------------------------------------------------------------------------------------
	//! Server: the opening's size, whether it is a window, the floor inside it, and the boards it starts with
	void Setup(float width, float height, int boards, bool window, vector inside)
	{
		m_fWidth = Math.Max(width, 0.5);
		m_fHeight = Math.Max(height, 0.5);
		m_bWindow = window;
		m_vInside = inside;
		m_iBoards = Math.ClampInt(boards, 0, GetMaxBoards());
		m_fTopBoardDamage = 0;
		Replication.BumpMe();
		RebuildBoards();
	}

	//------------------------------------------------------------------------------------------------
	int GetBoards()
	{
		return m_iBoards;
	}

	//------------------------------------------------------------------------------------------------
	//! The prefab's boards, plus any the Holdout upgrade adds
	int GetMaxBoards()
	{
		CRF_HoldoutGamemodeComponent holdout = CRF_HoldoutGamemodeComponent.GetInstance();
		if (holdout)
			return m_iMaxBoards + holdout.GetExtraBoards();

		return m_iMaxBoards;
	}

	//------------------------------------------------------------------------------------------------
	bool IsFull()
	{
		return m_iBoards >= GetMaxBoards();
	}

	//------------------------------------------------------------------------------------------------
	bool IsWindow()
	{
		return m_bWindow;
	}

	//------------------------------------------------------------------------------------------------
	//! Which way is out
	vector GetOutward()
	{
		vector transform[4];
		GetOwner().GetWorldTransform(transform);
		return transform[2];
	}

	//------------------------------------------------------------------------------------------------
	//! Whether a point is on the outside of this opening's wall
	bool IsOutside(vector position)
	{
		vector offset = position - GetOwner().GetOrigin();
		offset[1] = 0;
		return vector.Dot(offset, GetOutward()) > 0.2;
	}

	//------------------------------------------------------------------------------------------------
	//! Server: one board back on, put up by playerId (who is paid for it). False when it was already full.
	bool AddBoard(int playerId)
	{
		if (!Replication.IsServer() || IsFull())
			return false;

		m_iBoards++;
		m_fTopBoardDamage = 0;
		Replication.BumpMe();
		RebuildBoards();

		CRF_HoldoutGamemodeComponent holdout = CRF_HoldoutGamemodeComponent.GetInstance();
		if (holdout)
			holdout.OnBoardRepaired(playerId);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Server, once a second: zombies in reach wear the top board down, or come in through the bare opening
	protected void ServerTick()
	{
		CollectZombiesInReach();
		if (m_aZombiesInReach.IsEmpty())
			return;

		if (m_iBoards <= 0)
		{
			PassThrough();
			return;
		}

		float health = m_fBoardHealth;
		CRF_HoldoutGamemodeComponent holdout = CRF_HoldoutGamemodeComponent.GetInstance();
		if (holdout)
			health = health * holdout.GetBoardHealthScale();

		m_fTopBoardDamage += m_aZombiesInReach.Count() * m_fDamagePerZombie;
		if (m_fTopBoardDamage < health)
			return;

		m_fTopBoardDamage = 0;
		m_iBoards--;
		Replication.BumpMe();
		RebuildBoards();

		// The groups sent at it go for the defenders now rather than at the next retarget
		if (m_iBoards == 0 && holdout)
			holdout.OnBarricadeBroken(GetOwner());
	}

	//------------------------------------------------------------------------------------------------
	//! Zombies outside the bare opening go to the floor inside it: through a window whoever is first,
	//! through a doorway only those stuck on the threshold (the rest walk in by themselves)
	protected void PassThrough()
	{
		int limit = DOOR_PASS_PER_TICK;
		if (m_bWindow)
			limit = WINDOW_PASS_PER_TICK;

		vector inside = GetInsidePoint();
		int passed = 0;
		foreach (IEntity zombie : m_aZombiesInReach)
		{
			BaseGameEntity body = BaseGameEntity.Cast(zombie);
			if (!body || !IsOutside(body.GetOrigin()))
				continue;

			if (!m_bWindow && !IsStalled(body))
				continue;

			vector transform[4];
			body.GetWorldTransform(transform);
			transform[3] = inside;
			body.Teleport(transform);

			passed++;
			if (passed >= limit)
				return;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Where a zombie coming through lands: the point Setup was given, or (placed by hand) a metre in
	protected vector GetInsidePoint()
	{
		if (m_vInside != vector.Zero)
			return m_vInside;

		return GetOwner().GetOrigin() - GetOutward() * 1.0 + Vector(0, 0.05, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected void CollectZombiesInReach()
	{
		m_aZombiesInReach.Clear();

		// Low down, so a zombie standing below a window sill is still in reach; windows reach a little further
		float reach = m_fZombieReach;
		if (m_bWindow)
			reach = reach * 1.4;

		vector centre = GetOwner().GetOrigin() + Vector(0, 0.8, 0);
		GetGame().GetWorld().QueryEntitiesBySphere(centre, reach, CollectZombie, null, EQueryEntitiesFlags.DYNAMIC);
	}

	//------------------------------------------------------------------------------------------------
	protected bool CollectZombie(IEntity entity)
	{
		SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(entity);
		if (!character || character.GetCharacterController().IsDead())
			return true;

		Faction faction = character.GetFaction();
		if (faction && faction.GetFactionKey() == m_sZombieFactionKey && !m_aZombiesInReach.Contains(character))
			m_aZombiesInReach.Insert(character);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Every machine: the doors hanging in this opening go. The server removes replicated ones (that
	//! reaches everyone); a machine only removes its own copy of a door that isn't replicated.
	protected void RemoveDoors()
	{
		m_aDoorsFound.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(GetOwner().GetOrigin() + Vector(0, 1, 0), Math.Max(m_fWidth, 1.2), CollectDoor, null, EQueryEntitiesFlags.ALL);

		bool server = Replication.IsServer();
		foreach (IEntity door : m_aDoorsFound)
		{
			bool replicated = door.FindComponent(RplComponent) != null;
			if (server || !replicated)
				SCR_EntityHelper.DeleteEntityAndChildren(door);
		}
		m_aDoorsFound.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected bool CollectDoor(IEntity entity)
	{
		if (entity.FindComponent(DoorComponent) && !m_aDoorsFound.Contains(entity))
			m_aDoorsFound.Insert(entity);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Clients: the boards or size changed. The state a client streams a barricade in with (or joins
	//! mid-round to) doesn't always come with this call, so clients also re-check once a second;
	//! RebuildBoards only does anything when what it built no longer matches.
	protected void OnBoardsChanged()
	{
		RebuildBoards();
	}

	//------------------------------------------------------------------------------------------------
	//! Board rows, every three: bottom, top, then a diagonal brace across the middle, as shares of the
	//! opening's height (the panel's centre). Rows past three repeat, nudged outwards.
	protected static float GetRowHeight(int row)
	{
		switch (row % 3)
		{
			case 0: return 0.28;
			case 1: return 0.76;
		}

		return 0.52;
	}

	//------------------------------------------------------------------------------------------------
	protected static float GetRowRoll(int row)
	{
		if (row % 3 == 2)
			return 22;

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Every machine: board props for the current count, rebuilt only when the count or size changes
	protected void RebuildBoards()
	{
		if (m_iBuiltBoards == m_iBoards && m_fBuiltWidth == m_fWidth && m_fBuiltHeight == m_fHeight)
			return;

		ClearBoards();
		m_iBuiltBoards = m_iBoards;
		m_fBuiltWidth = m_fWidth;
		m_fBuiltHeight = m_fHeight;

		if (m_iBoards <= 0 || m_sBoardPrefab.IsEmpty())
			return;

		Resource resource = Resource.Load(m_sBoardPrefab);
		if (!resource || !resource.IsValid())
			return;

		vector owner[4];
		GetOwner().GetWorldTransform(owner);

		int columns = (int)Math.Max(1, Math.Ceil(m_fWidth / PANEL_WIDTH));
		for (int board = 0; board < m_iBoards; board++)
		{
			float depth = 0.04 * board;
			for (int column = 0; column < columns; column++)
			{
				float x = (column - (columns - 1) * 0.5) * PANEL_WIDTH;

				// Lying flat in its own frame: pitched up 90 degrees to stand across the opening
				vector local[4];
				Math3D.AnglesToMatrix(Vector(0, 90, GetRowRoll(board)), local);
				local[3] = Vector(x, GetRowHeight(board) * m_fHeight, depth);

				EntitySpawnParams spawnParams = new EntitySpawnParams();
				spawnParams.TransformMode = ETransformMode.WORLD;
				Math3D.MatrixMultiply4(owner, local, spawnParams.Transform);

				IEntity prop = GetGame().SpawnEntityPrefabLocal(resource, GetGame().GetWorld(), spawnParams);
				if (prop)
					m_aBoardProps.Insert(prop);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void ClearBoards()
	{
		foreach (IEntity prop : m_aBoardProps)
		{
			if (prop)
				SCR_EntityHelper.DeleteEntityAndChildren(prop);
		}

		m_aBoardProps.Clear();
		m_iBuiltBoards = -1;
	}
}
