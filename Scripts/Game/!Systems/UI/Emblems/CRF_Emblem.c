//------------------------------------------------------------------------------------------------
//! Draws a member's emblem - made on the website (coalitiongroup.net/emblem) - from the shape atlas that ships
//! with the CRF (UI/Images/Emblems, generated together with the website's shapes by
//! Website_Angular/tools/emblems/generate_emblem_shapes.py, so both look the same).
//!
//!   "1;<layer>;<layer>;..."  layer = shape,RRGGBB,x,y,w,h,rot,flags,opacity
//!   x, y centre 0-1000 · w, h 20-2000 (1000 = emblem width) · rot 0-359 clockwise · flags 1 = flip H, 2 = flip V
//!   opacity 1-100 · layers bottom to top
//!
//! Same format as the website (backend/src/services/emblemService.ts) - keep them in step.
class CRF_Emblem
{
	static const ResourceName IMAGESET = "{6B3E1A0C5D7F2E92}UI/Images/Emblems/CRF_EmblemShapes.imageset";
	protected static const ResourceName LAYER_LAYOUT = "{6B3E1A0C5D7F2E94}UI/layouts/Emblems/CRF_EmblemLayer.layout";
	protected static const string VERSION = "1";
	protected static const int MAX_LAYERS = 16;
	protected static const string HEX = "0123456789ABCDEF";

	//------------------------------------------------------------------------------------------------
	//! Draw an emblem into a (clipping) FrameWidget, replacing whatever it held
	//! \param[in] size the container's width and height, in layout units
	//! \return true when at least one layer was drawn
	static bool Draw(Widget container, string data, float size)
	{
		if (!container)
			return false;

		SCR_WidgetHelper.RemoveAllChildren(container);
		if (data.IsEmpty())
			return false;

		array<string> parts = {};
		data.Split(";", parts, true);
		if (parts.IsEmpty() || parts[0] != VERSION)
			return false;

		WorkspaceWidget workspace = GetGame().GetWorkspace();
		int drawn;
		for (int i = 1; i < parts.Count() && drawn < MAX_LAYERS; i++)
		{
			array<string> fields = {};
			parts[i].Split(",", fields, false);
			if (fields.Count() != 9)
				continue;

			ImageWidget image = ImageWidget.Cast(workspace.CreateWidgets(LAYER_LAYOUT, container));
			if (!image)
				continue;

			if (!image.LoadImageFromSet(0, IMAGESET, fields[0]))
			{
				image.RemoveFromHierarchy();
				continue;
			}

			// Website colours are sRGB; widget colours are linear
			int rgb = HexToInt(fields[1]);
			image.SetColor(Color.FromSRGBA((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF, 255));
			image.SetOpacity(Math.Clamp(fields[8].ToInt(), 1, 100) / 100.0);

			// Centred on (x, y), anchored to that point of the container
			float x = Math.Clamp(fields[2].ToInt(), 0, 1000) / 1000.0;
			float y = Math.Clamp(fields[3].ToInt(), 0, 1000) / 1000.0;
			float width = Math.Clamp(fields[4].ToInt(), 20, 2000) / 1000.0 * size;
			float height = Math.Clamp(fields[5].ToInt(), 20, 2000) / 1000.0 * size;
			FrameSlot.SetAnchorMin(image, x, y);
			FrameSlot.SetAnchorMax(image, x, y);
			FrameSlot.SetSize(image, width, height);
			FrameSlot.SetPos(image, -width / 2, -height / 2);

			image.SetPivot(0.5, 0.5);
			image.SetRotation(fields[6].ToInt());

			int flags = fields[7].ToInt();
			if (flags & 1)
				image.SetFlags(WidgetFlags.FLIPU);
			if (flags & 2)
				image.SetFlags(WidgetFlags.FLIPV);

			drawn++;
		}

		return drawn > 0;
	}

	//------------------------------------------------------------------------------------------------
	//! "RRGGBB" to 0xRRGGBB (white when it isn't hex)
	protected static int HexToInt(string hex)
	{
		hex.ToUpper();
		int value;
		for (int i = 0; i < hex.Length(); i++)
		{
			int digit = HEX.IndexOf(hex.Get(i));
			if (digit < 0)
				return 0xFFFFFF;

			value = value * 16 + digit;
		}

		return value;
	}
}
