//------------------------------------------------------------------------------------------------
//! Starts a downloaded replay: loads the replay world for its terrain (CRF_ReplayWorlds - the terrain plus
//! CRF_ReplayGameMode, nothing of the recorded mission) as a local game, and leaves the file to play in
//! GameSessionStorage, which survives the world change. CRF_ReplayGameMode picks it up when it starts.
class CRF_ReplayLauncher
{
	//------------------------------------------------------------------------------------------------
	//! \return an error to show, or empty when the world is loading
	static string Launch(int missionId)
	{
		CRF_ReplayMeta meta = CRF_ReplayLibrary.LoadMeta(missionId);
		if (!meta || !CRF_ReplayLibrary.IsComplete(missionId))
			return "Download this replay first";

		string world = CRF_ReplayWorlds.Get(meta.terrainKey);
		if (world.IsEmpty())
			return string.Format("No replay world for the terrain \"%1\" yet (CRF_ReplayWorlds)", meta.terrainKey);

		// No Resource.Load check here: a world .ent isn't a loadable resource, so it failed for every terrain.
		// Like vanilla, the scenario change below is the check - it refuses a world it can't start.
		GameSessionStorage.s_Data.Set(CRF_Replay.STORAGE_FILE, CRF_Replay.ReplayPath(missionId));
		GameSessionStorage.s_Data.Set(CRF_Replay.STORAGE_WORLD, world);
		GameSessionStorage.s_Data.Set(CRF_Replay.STORAGE_TITLE, meta.name);

		// The terrain's replay world itself - one scenario (Missions/Replay/CRF_Replays.conf) gets players here,
		// and the world switches to whichever terrain the replay was recorded on
		if (!GameStateTransitions.RequestScenarioChangeTransition(world, ResourceName.Empty, GetAddonList()))
		{
			ClearPending();
			return "The game couldn't load this replay's terrain - make sure its terrain mod is downloaded and enabled (" + world + ")";
		}

		GetGame().GetMenuManager().CloseAllMenus();
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
	//! A replay world has started (CRF_ReplayGameMode): play what the launcher asked for
	static void OnReplayWorldStarted()
	{
		if (CRF_ReplayPlayer.GetInstance())
			return;

		string file, title;
		if (GameSessionStorage.s_Data.Find(CRF_Replay.STORAGE_FILE, file))
		{
			GameSessionStorage.s_Data.Find(CRF_Replay.STORAGE_TITLE, title);
			ClearPending();

			// Straight away: until the replay's camera exists the game has nothing to show
			CRF_ReplayPlayer.Start(file, title);
			return;
		}

#ifdef WORKBENCH
		if (WorkbenchAutoplay())
			return;
#endif

		// Nothing chosen yet - the "COALITION Replays" scenario (Missions/Replay/CRF_Replays.conf): pick one here
		OpenHub();
	}

	//------------------------------------------------------------------------------------------------
	//! The replay browser over a slow look at the terrain. Mods don't run in Reforger's main menu, so this is
	//! where players choose a replay: Scenarios > COALITION Replays.
	protected static void OpenHub()
	{
		BaseWorld world = GetGame().GetWorld();
		vector mins, maxs;
		world.GetBoundBox(mins, maxs);
		vector centre = (mins + maxs) * 0.5;
		centre[1] = world.GetSurfaceY(centre[0], centre[2]) + 250;

		vector transform[4];
		Math3D.AnglesToMatrix(Vector(30, -25, 0), transform);
		transform[3] = centre;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform = transform;
		CameraBase camera = CameraBase.Cast(GetGame().SpawnEntityPrefab(Resource.Load(COA_EntityHelper.GetSpectatorCameraResource()), world, params));
		CameraManager cameraManager = GetGame().GetCameraManager();
		if (camera && cameraManager)
			cameraManager.SetCamera(camera);

		CRF_ReplayBrowserMenu.s_bInHub = true;
		GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.CRF_ReplayBrowser);
	}

#ifdef WORKBENCH
	//------------------------------------------------------------------------------------------------
	//! Workbench test session: play a replay world (Worlds/Replay/CRF_Replay_<terrain>.ent) and the newest
	//! downloaded replay recorded on that terrain starts by itself. A mission ID in
	//! $profile:vaar/replays/autoplay.txt picks a particular one.
	//! \return true when a replay started (otherwise the browser opens)
	protected static bool WorkbenchAutoplay()
	{
		string loadedWorld = WorldPath(GetGame().GetWorldFile());
		CRF_ReplayMeta chosen;

		string autoplayPath = CRF_Replay.DIR + "/autoplay.txt";
		if (FileIO.FileExists(autoplayPath))
		{
			FileHandle file = FileIO.OpenFile(autoplayPath, FileMode.READ);
			string line;
			if (file)
			{
				file.ReadLine(line);
				file.Close();
			}

			line = line.Trim();
			chosen = CRF_ReplayLibrary.LoadMeta(line.ToInt());
			if (!chosen)
			{
				Print(string.Format("[CRF_Replay] autoplay.txt names mission \"%1\" but there's no %2 - delete autoplay.txt to pick by terrain", line, CRF_Replay.MetaPath(line.ToInt())), LogLevel.WARNING);
				return false;
			}
		}
		else
		{
			foreach (CRF_ReplayMeta meta : CRF_ReplayLibrary.ListLocal())
			{
				if (meta.complete && WorldPath(CRF_ReplayWorlds.Get(meta.terrainKey)) == loadedWorld)
				{
					chosen = meta;
					break;
				}
			}

			if (!chosen)
			{
				Print("[CRF_Replay] Workbench autoplay: no downloaded replay in " + CRF_Replay.DIR + " was recorded on this terrain (" + loadedWorld + ") - opening the replay browser", LogLevel.NORMAL);
				return false;
			}
		}

		string world = CRF_ReplayWorlds.Get(chosen.terrainKey);
		if (WorldPath(world) != loadedWorld)
		{
			Print(string.Format("[CRF_Replay] Replay %1 (%2) was recorded on %3 - play %4 instead", chosen.id, chosen.name, chosen.terrainKey, world), LogLevel.WARNING);
			return false;
		}

		Print(string.Format("[CRF_Replay] Workbench autoplay: replay %1 - %2", chosen.id, chosen.name), LogLevel.NORMAL);
		CRF_ReplayPlayer.Start(CRF_Replay.ReplayPath(chosen.id), chosen.name);
		return true;
	}
#endif

	//------------------------------------------------------------------------------------------------
	//! "{GUID}Worlds/Replay/X.ent", "$COALITIONFramework:Worlds/Replay/X.ent" (GetWorldFile) and
	//! "worlds/replay/x.ent" compare equal
	protected static string WorldPath(string world)
	{
		int brace = world.IndexOf("}");
		if (brace >= 0)
			world = world.Substring(brace + 1, world.Length() - brace - 1);

		if (world.StartsWith("$"))
		{
			int colon = world.IndexOf(":");
			if (colon >= 0)
				world = world.Substring(colon + 1, world.Length() - colon - 1);
		}

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
	//! Workbench keeps script statics between plays
	override void OnGameEnd()
	{
		CRF_ReplayPlayer.Stop();
		super.OnGameEnd();
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
