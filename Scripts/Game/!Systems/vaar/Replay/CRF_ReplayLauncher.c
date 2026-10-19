//------------------------------------------------------------------------------------------------
//! Starts a downloaded replay: loads the mission's terrain as a local game (no mission, no game mode)
//! and leaves the file to play in GameSessionStorage, which survives the world change. When the terrain
//! has loaded, the game's world post-process (below) starts CRF_ReplayPlayer on it.
class CRF_ReplayLauncher
{
	//------------------------------------------------------------------------------------------------
	//! \return an error to show, or empty when the terrain is loading
	static string Launch(int missionId)
	{
		CRF_ReplayMeta meta = CRF_ReplayLibrary.LoadMeta(missionId);
		if (!meta || !CRF_ReplayLibrary.IsComplete(missionId))
			return "Download this replay first";

		string world = CRF_Replay.GetTerrainWorld(meta.terrainKey);
		if (world.IsEmpty())
			world = WorldFromHeader(CRF_Replay.ReplayPath(missionId));

		if (world.IsEmpty())
			return string.Format("No terrain known for \"%1\" - add it to CRF_Replay.GetTerrainWorld", meta.terrainKey);

		Resource resource = Resource.Load(world);
		if (!resource || !resource.IsValid())
			return "The terrain for this replay isn't installed (" + world + ")";

		GameSessionStorage.s_Data.Set(CRF_Replay.STORAGE_FILE, CRF_Replay.ReplayPath(missionId));
		GameSessionStorage.s_Data.Set(CRF_Replay.STORAGE_WORLD, world);
		GameSessionStorage.s_Data.Set(CRF_Replay.STORAGE_TITLE, meta.name);

		if (!GameStateTransitions.RequestScenarioChangeTransition(world, ResourceName.Empty, GetAddonList()))
		{
			ClearPending();
			return "The game refused to load " + world;
		}

		GetGame().GetMenuManager().CloseAllMenus();
		return "";
	}

	//------------------------------------------------------------------------------------------------
	//! Recordings from format v3 list the mission's world and its parent terrain under "scenes"
	protected static string WorldFromHeader(string path)
	{
		FileHandle file = FileIO.OpenFile(path, FileMode.READ);
		if (!file)
			return "";

		string header;
		file.ReadLine(header);
		file.Close();

		header = header.Trim();
		if (header.EndsWith("["))
			header += "]}";

		JsonLoadContext context = new JsonLoadContext(false);
		array<string> scenes = {};
		if (!context.LoadFromString(header) || !context.ReadValue("scenes", scenes))
			return "";

		// Any scene but the mission's own world (which would bring its game mode along) is the terrain
		string missionWorld;
		context.ReadValue("world", missionWorld);
		foreach (string scene : scenes)
		{
			if (scene.StartsWith("{") && WorldPath(scene) != WorldPath(missionWorld))
				return scene;
		}

		return "";
	}

	//------------------------------------------------------------------------------------------------
	//! The mods loaded now, so the terrain and the recorded prefabs come along
	protected static string GetAddonList()
	{
		string list;
		array<string> addons = {};
		GameProject.GetLoadedAddons(addons);
		foreach (string guid : addons)
		{
			if (GameProject.IsVanillaAddon(guid))
				continue;

			if (!list.IsEmpty())
				list += ",";

			list += guid;
		}

		return list;
	}

	//------------------------------------------------------------------------------------------------
	static void ClearPending()
	{
		GameSessionStorage.s_Data.Remove(CRF_Replay.STORAGE_FILE);
		GameSessionStorage.s_Data.Remove(CRF_Replay.STORAGE_WORLD);
		GameSessionStorage.s_Data.Remove(CRF_Replay.STORAGE_TITLE);
	}

	//------------------------------------------------------------------------------------------------
	//! Called when a world has loaded: starts the replay if this is the terrain one was launched on
	static void OnWorldLoaded()
	{
		string file, world, title;
		if (!GameSessionStorage.s_Data.Find(CRF_Replay.STORAGE_FILE, file))
			return;

		GameSessionStorage.s_Data.Find(CRF_Replay.STORAGE_WORLD, world);
		GameSessionStorage.s_Data.Find(CRF_Replay.STORAGE_TITLE, title);
		ClearPending();

		// Any other world (the main menu, a server) - the replay was abandoned
		if (WorldPath(GetGame().GetWorldFile()) != WorldPath(world))
			return;

		// Let the world finish initialising before spawning into it
		GetGame().GetCallqueue().CallLater(CRF_ReplayPlayer.Start, 500, false, file, title);
	}

	//------------------------------------------------------------------------------------------------
	//! "{GUID}worlds/Eden/Eden.ent" and "worlds/eden/eden.ent" compare equal
	protected static string WorldPath(string world)
	{
		int brace = world.IndexOf("}");
		if (brace >= 0)
			world = world.Substring(brace + 1, world.Length() - brace - 1);

		world.ToLower();
		world.Replace("\\", "/");
		return world;
	}

	//------------------------------------------------------------------------------------------------
	//! Back to the main menu
	static void Exit()
	{
		CRF_ReplayPlayer.Stop();
		ClearPending();
		GameStateTransitions.RequestGameplayEndTransition();
	}
}

//------------------------------------------------------------------------------------------------
modded class ArmaReforgerScripted
{
	//------------------------------------------------------------------------------------------------
	protected override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);
		CRF_ReplayLauncher.OnWorldLoaded();
	}
}

//------------------------------------------------------------------------------------------------
//! The Lobby camera only takes input under its own spectator and editor menus - let it fly under the
//! replay HUD as well
modded class COA_SpectatorCamera
{
	//------------------------------------------------------------------------------------------------
	override protected bool IsDisabledByMenu()
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		if (menuManager && !menuManager.IsAnyDialogOpen() && CRF_ReplayHUDMenu.Cast(menuManager.GetTopMenu()))
			return false;

		return super.IsDisabledByMenu();
	}
}
