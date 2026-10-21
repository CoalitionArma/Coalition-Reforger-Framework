#ifdef WORKBENCH
//------------------------------------------------------------------------------------------------
//! Exports the open world's terrain and the shapes of its buildings, walls, rocks and trees for the
//! website's 3D AAR playback (public/map-viewer, 3D mode). Run it once per terrain in the World Editor,
//! then upload the output folder to the server next to that terrain's map tiles:
//!   /map-viewer/terrains/<terrain key>/3d/
//!
//! Output (all heights stored as uint16 (height + 100 m) × 10, little-endian):
//!   meta.json                 sizes, spacings, tile grid
//!   coarse.bin                the whole terrain at the coarse spacing, rows along X, row by row along Z
//!   h_<tx>_<tz>.bin           one tile (TILE_SIZE m square) at the fine spacing, samples on both edges
//!   o_<tx>_<tz>.json          buildings / walls / rocks in the tile: [x, z, baseY, sizeX, sizeZ, height, yaw]
//!                             (decimetres, yaw in tenths of a degree clockwise from north) per kind
//!   t_<tx>_<tz>.bin           trees in the tile: 5 × uint16 each - x, z (decimetres from the tile corner),
//!                             base height (encoded like the terrain), height and crown radius (decimetres)
[WorkbenchPluginAttribute(
	name: "CRF VAAR 3D Terrain Export",
	description: "Exports the open terrain's heights and object shapes for the website's 3D AAR playback",
	wbModules: { "WorldEditor" },
	category: "CRF Mission Plugins",
	awesomeFontCode: 0xF1B2)] // cube
class CRF_VAAR3D_TerrainExportPlugin : WorkbenchPlugin
{
	[Attribute("", desc: "The viewer's terrain key for this world (TERRAIN_CONFIGS in viewer.html), e.g. chernarus, arland, eden")]
	protected string m_sTerrainKey;

	[Attribute("$profile:vaar3d", UIWidgets.FileNamePicker, desc: "Folder the export goes to (a sub-folder named after the terrain key is created)")]
	protected string m_sOutputDir;

	[Attribute("16", desc: "Coarse whole-terrain grid spacing [m]", params: "4 64 1")]
	protected float m_fCoarseSpacing;

	[Attribute("2", desc: "Fine tile grid spacing [m]. 2 m gives sharp terrain; 4 m quarters the export size and time.", params: "1 8 1")]
	protected float m_fTileSpacing;

	[Attribute("1", desc: "Export buildings, walls, rocks and trees")]
	protected bool m_bExportObjects;

	[Attribute("0 0 0", desc: "Only export fine tiles from here (X, -, Z) - leave both corners at 0 for the whole terrain")]
	protected vector m_vAreaMin;

	[Attribute("0 0 0", desc: "...to here (X, -, Z)")]
	protected vector m_vAreaMax;

	[Attribute("0", desc: "Export all terrains: also redo terrains that already have an export")]
	protected bool m_bOverwrite;

	protected static const string PLUGIN_NAME = "CRF VAAR 3D Terrain Export";
	protected static const float TILE_SIZE = 1024;
	protected static const float HEIGHT_OFFSET = 100;
	protected static const float HEIGHT_SCALE = 10;
	protected static const float MIN_TREE_HEIGHT = 2.5;
	protected static const float MIN_STRUCTURE_HEIGHT = 0.8;
	protected static const float MIN_STRUCTURE_FOOTPRINT = 1.5;	// m², anything smaller is clutter
	protected static const float MIN_ROCK_SIZE = 2;

	protected WorldEditorAPI m_Api;
	protected string m_sDir;

	// Objects found by the current tile's query
	protected float m_fTileX;
	protected float m_fTileZ;
	protected ref array<string> m_aBuildings = {};
	protected ref array<string> m_aWalls = {};
	protected ref array<string> m_aRocks = {};
	protected ref array<int> m_aTrees = {};
	protected int m_iObjectTotal;
	protected int m_iTreeTotal;

	//! Export all terrains: viewer terrain key and the terrain world (the parent of CRF's mission worlds)
	protected static const ref array<string> ALL_TERRAINS = {
		"arland", "{A9806AF617972E97}worlds/Arland/Arland.ent",
		"eden", "{853E92315D1D9EFE}worlds/Eden/Eden.ent",
		"kolguyev", "{1EA95DAE3230BEB0}worlds/Cain/Cain.ent",
		"chernarus", "{F513898A573B9C3F}Worlds/ChernarusMinusBeta.ent",
		"belleauwood", "{340ED3C1F743457E}Worlds/BelleauWood/BelleauWood.ent",
		"belize", "{D225252FA26D6D54}Worlds/UKSFO_S_Belize.ent",
		"falklands", "{B3AC4C35D795749C}Worlds/BritMapProject/BritMapProject.ent",
		"kunar", "{B70B908EF90E5F3A}Worlds/Kunar.ent",
		"kupyansk", "{9194858F3102F45C}worlds/KupyanskWorld.ent",
		"mogadishu", "{BCD52EA0F63F6EB3}Worlds/Mogadishu/mogadishu.ent",
		"normandy", "{55B6E6A52487C726}Worlds/TESTIsland.ent",
		"serhiivka", "{BC070CAEF616045B}Worlds/Serhiivka.ent",
		"vindrindop", "{1BE6FFB423CA6165}Worlds/VinDrinDop.ent",
		"wakeisland", "{2C7D70186FB9981F}Project Tropic.ent",
		"yeonpyeong", "{9D726E3A65816832}Yeonpyeong.ent"
	};

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		m_Api = worldEditor.GetApi();
		if (!m_Api || !m_Api.GetWorld())
		{
			Workbench.Dialog(PLUGIN_NAME, "Open the terrain's world in the World Editor first.");
			return;
		}

		int choice = Workbench.ScriptDialog(PLUGIN_NAME, "Exports terrain heights and object shapes for the 3D AAR.\nUpload the output folder to /map-viewer/terrains/<terrain key>/3d/ on the server.\n\nExport all terrains opens every COALITION map in turn (save your work first) - the terrain key and area are ignored.", this);
		if (choice == 0)
			return;

		if (choice == 2)
		{
			ExportAll(worldEditor);
			return;
		}

		m_sTerrainKey = m_sTerrainKey.Trim();
		m_sTerrainKey.ToLower();
		if (m_sTerrainKey.IsEmpty())
		{
			Workbench.Dialog(PLUGIN_NAME, "Set the terrain key (the viewer's name for this map, e.g. chernarus).");
			return;
		}

		string result = Export(worldEditor);
		Workbench.Dialog(PLUGIN_NAME, result);
	}

	//------------------------------------------------------------------------------------------------
	//! Every terrain in ALL_TERRAINS whose mod is loaded, one after another
	protected void ExportAll(WorldEditor worldEditor)
	{
		m_vAreaMin = vector.Zero;
		m_vAreaMax = vector.Zero;
		array<string> report = {};
		for (int i = 0; i < ALL_TERRAINS.Count(); i += 2)
		{
			string key = ALL_TERRAINS[i];
			string world = ALL_TERRAINS[i + 1];
			if (!m_bOverwrite && FileIO.FileExists(m_sOutputDir + "/" + key + "/meta.json"))
			{
				report.Insert(key + ": already exported");
				continue;
			}

			Resource resource = Resource.Load(world);
			if (!resource || !resource.IsValid())
			{
				report.Insert(key + ": skipped - its mod isn't loaded");
				continue;
			}

			Print("[CRF_VAAR3D] Opening " + world, LogLevel.NORMAL);
			if (!worldEditor.SetOpenedResource(world))
			{
				report.Insert(key + ": couldn't open " + world);
				continue;
			}

			Sleep(500);
			m_Api = worldEditor.GetApi();
			if (!m_Api || !m_Api.GetWorld())
			{
				report.Insert(key + ": the world didn't load");
				continue;
			}

			m_sTerrainKey = key;
			m_iObjectTotal = 0;
			m_iTreeTotal = 0;
			string result = Export(worldEditor);
			Print("[CRF_VAAR3D] " + key + ": " + result, LogLevel.NORMAL);
			report.Insert(key + ": " + result);
		}

		string summary;
		foreach (string line : report)
		{
			summary += line + "\n";
		}

		Workbench.Dialog(PLUGIN_NAME, "Export all terrains\n\n" + summary);
	}

	//------------------------------------------------------------------------------------------------
	//! \return what happened, for the dialog
	protected string Export(WorldEditor worldEditor)
	{
		float unit = m_Api.GetTerrainUnitScale();
		float sizeX = m_Api.GetTerrainResolutionX() * unit;
		float sizeZ = m_Api.GetTerrainResolutionY() * unit;
		if (sizeX <= 0 || sizeZ <= 0)
			return "This world has no terrain to export.";

		m_sDir = m_sOutputDir + "/" + m_sTerrainKey;
		FileIO.MakeDirectory(m_sOutputDir);
		FileIO.MakeDirectory(m_sDir);

		WBProgressDialog progress = new WBProgressDialog(PLUGIN_NAME, worldEditor);
		Debug.BeginTimeMeasure();

		// Coarse grid over everything
		progress.SetActionTitle("Coarse terrain");
		int coarseW = Math.Ceil(sizeX / m_fCoarseSpacing) + 1;
		int coarseH = Math.Ceil(sizeZ / m_fCoarseSpacing) + 1;
		if (!WriteGrid(m_sDir + "/coarse.bin", 0, 0, m_fCoarseSpacing, coarseW, coarseH))
			return "Could not write to " + m_sDir;

		// Fine tiles
		int cols = Math.Ceil(sizeX / TILE_SIZE);
		int rows = Math.Ceil(sizeZ / TILE_SIZE);
		int samples = Math.Round(TILE_SIZE / m_fTileSpacing) + 1;
		bool limited = m_vAreaMax[0] > m_vAreaMin[0] && m_vAreaMax[2] > m_vAreaMin[2];

		array<string> written = {};
		int done;
		int total = cols * rows;
		for (int tz; tz < rows; tz++)
		{
			for (int tx; tx < cols; tx++)
			{
				done++;
				progress.SetProgress(done / (float)total);
				float x0 = tx * TILE_SIZE;
				float z0 = tz * TILE_SIZE;
				if (limited && (x0 + TILE_SIZE < m_vAreaMin[0] || x0 > m_vAreaMax[0] || z0 + TILE_SIZE < m_vAreaMin[2] || z0 > m_vAreaMax[2]))
					continue;

				progress.SetActionTitle(string.Format("Tile %1, %2 of %3 x %4", tx, tz, cols, rows));
				string suffix = string.Format("%1_%2", tx, tz);
				WriteGrid(m_sDir + "/h_" + suffix + ".bin", x0, z0, m_fTileSpacing, samples, samples);
				if (m_bExportObjects)
					ExportObjects(tx, tz, suffix);

				written.Insert(string.Format("[%1,%2]", tx, tz));
			}
		}

		// meta.json
		FileHandle meta = FileIO.OpenFile(m_sDir + "/meta.json", FileMode.WRITE);
		if (meta)
		{
			string worldPath;
			m_Api.GetWorldPath(worldPath);
			worldPath.Replace("\\", "/");
			worldPath.Replace("\"", "'");
			meta.WriteLine("{");
			meta.WriteLine("  \"v\": 1,");
			meta.WriteLine(string.Format("  \"terrain\": \"%1\",", m_sTerrainKey));
			meta.WriteLine(string.Format("  \"world\": \"%1\",", worldPath));
			meta.WriteLine(string.Format("  \"size\": [%1, %2],", (int)sizeX, (int)sizeZ));
			meta.WriteLine(string.Format("  \"heightOffset\": %1,", HEIGHT_OFFSET));
			meta.WriteLine(string.Format("  \"heightScale\": %1,", HEIGHT_SCALE));
			meta.WriteLine(string.Format("  \"coarse\": { \"spacing\": %1, \"w\": %2, \"h\": %3 },", m_fCoarseSpacing, coarseW, coarseH));
			meta.WriteLine(string.Format("  \"tile\": { \"size\": %1, \"spacing\": %2, \"samples\": %3, \"cols\": %4, \"rows\": %5 },", TILE_SIZE, m_fTileSpacing, samples, cols, rows));
			meta.WriteLine(string.Format("  \"objects\": %1,", m_bExportObjects));
			meta.WriteLine("  \"tiles\": [" + JoinStrings(written) + "]");
			meta.WriteLine("}");
			meta.Close();
		}

		Debug.EndTimeMeasure("[CRF_VAAR3D] Export " + m_sTerrainKey);
		return string.Format("Exported %1 tiles, %2 structures and %3 trees to %4 - upload that folder to /map-viewer/terrains/%5/3d/ on the server.",
			written.Count(), m_iObjectTotal, m_iTreeTotal, m_sDir, m_sTerrainKey);
	}

	//------------------------------------------------------------------------------------------------
	//! Heights on a w × h grid from (x0, z0), row by row along Z
	protected bool WriteGrid(string path, float x0, float z0, float spacing, int w, int h)
	{
		FileHandle file = FileIO.OpenFile(path, FileMode.WRITE);
		if (!file)
			return false;

		array<int> row = {};
		row.Resize(w);
		for (int j; j < h; j++)
		{
			float z = z0 + j * spacing;
			for (int i; i < w; i++)
				row[i] = EncodeHeight(m_Api.GetTerrainSurfaceY(x0 + i * spacing, z));

			file.WriteArray(row, 2, w);
		}

		file.Close();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected int EncodeHeight(float y)
	{
		return Math.ClampInt(Math.Round((y + HEIGHT_OFFSET) * HEIGHT_SCALE), 0, 65535);
	}

	//------------------------------------------------------------------------------------------------
	protected void ExportObjects(int tx, int tz, string suffix)
	{
		m_fTileX = tx * TILE_SIZE;
		m_fTileZ = tz * TILE_SIZE;
		m_aBuildings.Clear();
		m_aWalls.Clear();
		m_aRocks.Clear();
		m_aTrees.Clear();

		m_Api.GetWorld().QueryEntitiesByAABB(Vector(m_fTileX, -500, m_fTileZ), Vector(m_fTileX + TILE_SIZE, 3000, m_fTileZ + TILE_SIZE), AddEntity, null, EQueryEntitiesFlags.STATIC);

		if (!m_aBuildings.IsEmpty() || !m_aWalls.IsEmpty() || !m_aRocks.IsEmpty())
		{
			FileHandle file = FileIO.OpenFile(m_sDir + "/o_" + suffix + ".json", FileMode.WRITE);
			if (file)
			{
				file.Write(string.Format("{\"b\":[%1],\"w\":[%2],\"r\":[%3]}", JoinStrings(m_aBuildings), JoinStrings(m_aWalls), JoinStrings(m_aRocks)));
				file.Close();
			}
		}

		if (!m_aTrees.IsEmpty())
		{
			FileHandle trees = FileIO.OpenFile(m_sDir + "/t_" + suffix + ".bin", FileMode.WRITE);
			if (trees)
			{
				trees.WriteArray(m_aTrees, 2, m_aTrees.Count());
				trees.Close();
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected bool AddEntity(IEntity entity)
	{
		vector mins, maxs;
		entity.GetBounds(mins, maxs);
		float scale = entity.GetScale();
		vector size = (maxs - mins) * scale;
		vector center = entity.CoordToParent((mins + maxs) * 0.5);
		if (entity.GetParent())
			center = entity.GetParent().CoordToParent(center);

		// Each object goes to the tile its centre is in, so neighbouring tiles don't both draw it
		if (center[0] < m_fTileX || center[0] >= m_fTileX + TILE_SIZE || center[2] < m_fTileZ || center[2] >= m_fTileZ + TILE_SIZE)
			return true;

		if (entity.IsInherited(Tree))
		{
			if (size[1] < MIN_TREE_HEIGHT)
				return true;

			m_aTrees.Insert(Math.ClampInt(Math.Round((center[0] - m_fTileX) * 10), 0, 65535));
			m_aTrees.Insert(Math.ClampInt(Math.Round((center[2] - m_fTileZ) * 10), 0, 65535));
			m_aTrees.Insert(EncodeHeight(center[1] - size[1] * 0.5));
			m_aTrees.Insert(Math.ClampInt(Math.Round(size[1] * 10), 0, 65535));
			m_aTrees.Insert(Math.ClampInt(Math.Round(Math.Max(size[0], size[2]) * 5), 0, 65535));
			m_iTreeTotal++;
			return true;
		}

		// Structures: root entities only (their parts are children)
		if (entity.GetParent())
			return true;

		string path = GetPrefabPath(entity);
		if (path.IsEmpty())
			return true;

		path.ToLower();
		bool rock = path.Contains("prefabs/rocks/");
		bool structure = rock || entity.IsInherited(Building) || path.Contains("prefabs/structures/");
		if (!structure)
			return true;

		// Thin or tiny clutter isn't worth a box
		if (path.Contains("/signs/") || path.Contains("lamp") || path.Contains("/poles/") || path.Contains("powerline") || path.Contains("/wires/")
			|| path.Contains("/cables/") || path.Contains("/lights/") || path.Contains("/pipes/"))
			return true;

		if (size[1] < MIN_STRUCTURE_HEIGHT)
			return true;

		int cx = Math.Round(center[0] * 10);
		int cz = Math.Round(center[2] * 10);
		int baseY = Math.Round((center[1] - size[1] * 0.5) * 10);
		int sx = Math.Round(size[0] * 10);
		int sz = Math.Round(size[2] * 10);
		int sy = Math.Round(size[1] * 10);
		int yaw = Math.Round(entity.GetYawPitchRoll()[0] * 10);
		string record = string.Format("[%1,%2,%3,%4,%5,%6,%7]", cx, cz, baseY, sx, sz, sy, yaw);

		if (rock)
		{
			if (Math.Max(size[0], Math.Max(size[1], size[2])) < MIN_ROCK_SIZE)
				return true;

			m_aRocks.Insert(record);
		}
		else if (path.Contains("/walls/") || path.Contains("/fences/") || path.Contains("wall") || path.Contains("fence"))
		{
			m_aWalls.Insert(record);
		}
		else
		{
			if (size[0] * size[2] < MIN_STRUCTURE_FOOTPRINT)
				return true;

			m_aBuildings.Insert(record);
		}

		m_iObjectTotal++;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected string GetPrefabPath(IEntity entity)
	{
		EntityPrefabData prefabData = entity.GetPrefabData();
		if (!prefabData)
			return string.Empty;

		return prefabData.GetPrefabName();
	}

	//------------------------------------------------------------------------------------------------
	protected string JoinStrings(array<string> parts)
	{
		string joined;
		foreach (int i, string part : parts)
		{
			if (i > 0)
				joined += ",";

			joined += part;
		}

		return joined;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Export", true)]
	protected int ButtonExport()
	{
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Export all terrains")]
	protected int ButtonExportAll()
	{
		return 2;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Cancel")]
	protected int ButtonCancel()
	{
		return 0;
	}
}
#endif // WORKBENCH
