modded enum ChimeraMenuPreset
{
	CRF_AARReviewMenu
}

//------------------------------------------------------------------------------------------------
//! The coalitiongroup.net "Submit Your AAR" form, in game. Opened from the rating card on the AAR
//! screen (COA_AARMenu). Same questions and rules as the website; the server posts it to the
//! website on the player's behalf (CRF_AARManager.SubmitReview). Closing keeps unsent edits as a
//! draft for the rest of the round.
class CRF_AARReviewMenu : ChimeraMenuBase
{
	protected static const ResourceName STAR_ROW_LAYOUT = "{6A6D100000000002}UI/layouts/Menus/AAR/CRF_AARReviewStarRow.layout";
	protected static const ResourceName LEADER_CARD_LAYOUT = "{6A6D100000000003}UI/layouts/Menus/AAR/CRF_AARReviewLeaderCard.layout";

	protected ref COA_MenuPolish m_UIPolish;
	protected InputManager m_InputManager;

	protected Widget m_wRoot;
	protected MultilineEditBoxWidget m_wWentWell;
	protected MultilineEditBoxWidget m_wWentBad;
	protected MultilineEditBoxWidget m_wFeedback;
	protected VerticalLayoutWidget m_wCategoryRows;
	protected VerticalLayoutWidget m_wLeaderCards;
	protected TextWidget m_wOverall;
	protected TextWidget m_wNoLeadersHint;
	protected TextWidget m_wStatus;
	protected TextWidget m_wSubmitText;

	protected ref array<Widget> m_aCategoryRows = {};
	protected ref array<int> m_aCategoryValues = {0, 0, 0, 0, 0};
	protected ref array<ref CRF_AARLeaderCardWidgets> m_aCards = {};

	protected bool m_bSubmitting;
	protected bool m_bAppliedSaved;		// the saved review (or a draft) is already in the form

	//------------------------------------------------------------------------------------------------
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		m_wRoot = GetRootWidget();
		m_InputManager = GetGame().GetInputManager();

		m_wWentWell = MultilineEditBoxWidget.Cast(m_wRoot.FindAnyWidget("WentWell"));
		m_wWentBad = MultilineEditBoxWidget.Cast(m_wRoot.FindAnyWidget("WentBad"));
		m_wFeedback = MultilineEditBoxWidget.Cast(m_wRoot.FindAnyWidget("Feedback"));
		m_wCategoryRows = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("CategoryRows"));
		m_wLeaderCards = VerticalLayoutWidget.Cast(m_wRoot.FindAnyWidget("LeaderCards"));
		m_wOverall = TextWidget.Cast(m_wRoot.FindAnyWidget("OverallRating"));
		m_wNoLeadersHint = TextWidget.Cast(m_wRoot.FindAnyWidget("NoLeadersHint"));
		m_wStatus = TextWidget.Cast(m_wRoot.FindAnyWidget("StatusText"));

		Widget submitFrame = m_wRoot.FindAnyWidget("SubmitButtonFrame");
		if (submitFrame)
			m_wSubmitText = TextWidget.Cast(submitFrame.FindAnyWidget("ButtonText"));

		TextWidget missionName = TextWidget.Cast(m_wRoot.FindAnyWidget("MissionName"));
		if (missionName)
			missionName.SetText(GetGame().GetMissionName());

		GetButtonClick(m_wRoot, "AddLeader").Insert(OnAddLeaderClicked);
		GetButtonClick(m_wRoot, "SubmitButton").Insert(OnSubmitClicked);
		GetButtonClick(m_wRoot, "CancelButton").Insert(OnCancelClicked);

		for (int i = 0; i < CRF_AARReviewData.CATEGORY_COUNT; i++)
		{
			Widget row = CreateStarRow(m_wCategoryRows, CRF_AARReviewData.GetCategoryLabel(i));
			m_aCategoryRows.Insert(row);
		}

		CRF_AARReviewSession.SyncMission();
		CRF_AARReviewSession.s_OnFormReceived.Insert(OnFormReceived);
		CRF_AARReviewSession.s_OnSubmitResult.Insert(OnSubmitResult);

		if (CRF_AARReviewSession.s_Draft)
		{
			ApplyReview(CRF_AARReviewSession.s_Draft);
			m_bAppliedSaved = true;
		}
		else if (CRF_AARReviewSession.s_bHasReview && CRF_AARReviewSession.s_Review)
		{
			ApplyReview(CRF_AARReviewSession.s_Review);
			m_bAppliedSaved = true;
		}

		if (!CRF_AARReviewSession.s_bFormRequested)
		{
			CRF_AARReviewSession.s_bFormRequested = true;
			COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
			if (authorityManager)
				authorityManager.RequestAARReviewForm();

			SetStatus("Loading your saved AAR...", false);
		}
		else
		{
			ShowFormStatus();
		}

		RefreshOverall();
		RefreshLeaderHint();
		RefreshSubmitLabel();

		m_UIPolish = new COA_MenuPolish(m_wRoot, true);
		m_InputManager.AddActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnBack);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuUpdate(float tDelta)
	{
		super.OnMenuUpdate(tDelta);
		if (m_UIPolish)
			m_UIPolish.Update(tDelta);
	}

	//------------------------------------------------------------------------------------------------
	override void OnMenuClose()
	{
		// Keep the form as it was left for the rest of the round (reopening shows it again)
		CRF_AARReviewSession.s_Draft = Collect();

		CRF_AARReviewSession.s_OnFormReceived.Remove(OnFormReceived);
		CRF_AARReviewSession.s_OnSubmitResult.Remove(OnSubmitResult);
		if (m_InputManager)
			m_InputManager.RemoveActionListener(UIConstants.MENU_ACTION_BACK, EActionTrigger.DOWN, OnBack);

		if (m_UIPolish)
		{
			m_UIPolish.Cleanup();
			m_UIPolish = null;
		}

		super.OnMenuClose();
	}

	//------------------------------------------------------------------------------------------------
	//! Click event of a named button under parent (a throwaway invoker when it's missing, so callers
	//! can Insert without null checks)
	protected ScriptInvoker GetButtonClick(Widget parent, string name)
	{
		Widget button;
		if (parent)
			button = parent.FindAnyWidget(name);

		SCR_ButtonTextComponent component;
		if (button)
			component = SCR_ButtonTextComponent.Cast(button.FindHandler(SCR_ButtonTextComponent));

		if (!component)
			return new ScriptInvoker();

		return component.m_OnClicked;
	}

	//------------------------------------------------------------------------------------------------
	//! "Label  [1][2][3][4][5]" row; the pips report clicks to OnPipClicked
	protected Widget CreateStarRow(Widget parent, string label)
	{
		if (!parent)
			return null;

		Widget row = GetGame().GetWorkspace().CreateWidgets(STAR_ROW_LAYOUT, parent);
		if (!row)
			return null;

		TextWidget labelText = TextWidget.Cast(row.FindAnyWidget("StarLabel"));
		if (labelText)
			labelText.SetText(label);

		for (int i = 1; i <= 5; i++)
			GetButtonClick(row, "Pip" + i).Insert(OnPipClicked);

		SetRowValue(row, 0);
		return row;
	}

	//------------------------------------------------------------------------------------------------
	protected void SetRowValue(Widget row, int value)
	{
		if (!row)
			return;

		for (int i = 1; i <= 5; i++)
		{
			Widget pip = row.FindAnyWidget("PipBG" + i);
			if (!pip)
				continue;

			if (i <= value)
				pip.SetColor(Color.FromSRGBA(201, 54, 54, 255));
			else
				pip.SetColor(Color.FromSRGBA(28, 31, 40, 255));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected Widget FindAncestor(Widget widget, string name)
	{
		while (widget)
		{
			if (widget.GetName() == name)
				return widget;

			widget = widget.GetParent();
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected CRF_AARLeaderCardWidgets FindCard(Widget widget)
	{
		Widget root = FindAncestor(widget, "LeaderCardRoot");
		foreach (CRF_AARLeaderCardWidgets card : m_aCards)
		{
			if (card.m_wRoot == root)
				return card;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPipClicked(SCR_ButtonBaseComponent button)
	{
		if (!button || !button.GetRootWidget())
			return;

		// Pips are named Pip1 .. Pip5
		string name = button.GetRootWidget().GetName();
		int value = name.Substring(name.Length() - 1, 1).ToInt();
		Widget row = FindAncestor(button.GetRootWidget(), "StarRowRoot");
		if (value < 1 || value > 5 || !row)
			return;

		int categoryIndex = m_aCategoryRows.Find(row);
		if (categoryIndex >= 0)
		{
			m_aCategoryValues[categoryIndex] = value;
			SetRowValue(row, value);
			RefreshOverall();
			return;
		}

		CRF_AARLeaderCardWidgets card = FindCard(row);
		if (!card)
			return;

		int leaderIndex = card.m_aRows.Find(row);
		if (leaderIndex < 0)
			return;

		card.m_aScores[leaderIndex] = value;
		SetRowValue(row, value);
		RefreshCardOverall(card);
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshOverall()
	{
		if (!m_wOverall)
			return;

		int average = CRF_AARReviewData.Average(m_aCategoryValues);
		if (average > 0)
		{
			m_wOverall.SetText(string.Format("OVERALL  %1/5", average));
			m_wOverall.SetColor(Color.FromSRGBA(239, 242, 247, 255));
		}
		else
		{
			m_wOverall.SetText("OVERALL  –");
			m_wOverall.SetColor(Color.FromSRGBA(169, 180, 204, 255));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshCardOverall(CRF_AARLeaderCardWidgets card)
	{
		if (!card.m_wOverall)
			return;

		int average = CRF_AARReviewData.Average(card.m_aScores);
		if (average > 0)
		{
			card.m_wOverall.SetText(string.Format("%1/5", average));
			card.m_wOverall.SetColor(Color.FromSRGBA(239, 242, 247, 255));
		}
		else
		{
			card.m_wOverall.SetText("–");
			card.m_wOverall.SetColor(Color.FromSRGBA(169, 180, 204, 255));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshLeaderHint()
	{
		if (m_wNoLeadersHint)
			m_wNoLeadersHint.SetVisible(m_aCards.IsEmpty());
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshSubmitLabel()
	{
		if (!m_wSubmitText)
			return;

		if (m_bSubmitting)
			m_wSubmitText.SetText("SENDING...");
		else if (CRF_AARReviewSession.s_bHasReview)
			m_wSubmitText.SetText("UPDATE AAR");
		else
			m_wSubmitText.SetText("SUBMIT AAR");
	}

	//------------------------------------------------------------------------------------------------
	protected void SetStatus(string message, bool isError)
	{
		if (!m_wStatus)
			return;

		m_wStatus.SetText(message);
		if (isError)
			m_wStatus.SetColor(Color.FromSRGBA(232, 96, 96, 255));
		else
			m_wStatus.SetColor(Color.FromSRGBA(169, 180, 204, 255));
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowFormStatus()
	{
		if (!CRF_AARReviewSession.s_bLinked)
			SetStatus("Your Arma GUID isn't linked to a coalitiongroup.net account. Add it on your website profile so this AAR can be saved.", true);
		else if (CRF_AARReviewSession.s_bHasReview)
			SetStatus("You already submitted an AAR for this round - changes here update it.", false);
		else
			SetStatus("", false);
	}

	//------------------------------------------------------------------------------------------------
	// Leader cards
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected CRF_AARLeaderCardWidgets AddLeaderCard()
	{
		if (!m_wLeaderCards || m_aCards.Count() >= CRF_AARReviewData.MAX_LEADERS)
			return null;

		Widget root = GetGame().GetWorkspace().CreateWidgets(LEADER_CARD_LAYOUT, m_wLeaderCards);
		if (!root)
			return null;

		CRF_AARLeaderCardWidgets card = new CRF_AARLeaderCardWidgets();
		card.m_wRoot = root;
		card.m_wName = EditBoxWidget.Cast(root.FindAnyWidget("LeaderName"));
		card.m_wReason = MultilineEditBoxWidget.Cast(root.FindAnyWidget("LeaderReason"));
		card.m_wOverall = TextWidget.Cast(root.FindAnyWidget("LeaderOverall"));

		Widget ratings = root.FindAnyWidget("LeaderRatings");
		for (int i = 0; i < CRF_AARReviewData.CATEGORY_COUNT; i++)
			card.m_aRows.Insert(CreateStarRow(ratings, CRF_AARReviewData.GetLeaderCategoryLabel(i)));

		GetButtonClick(root, "PrevLeader").Insert(OnPrevLeaderClicked);
		GetButtonClick(root, "NextLeader").Insert(OnNextLeaderClicked);
		GetButtonClick(root, "RemoveLeader").Insert(OnRemoveLeaderClicked);
		GetButtonClick(root, "KarmaUp").Insert(OnKarmaUpClicked);
		GetButtonClick(root, "KarmaDown").Insert(OnKarmaDownClicked);

		m_aCards.Insert(card);
		RefreshCardOverall(card);
		RefreshKarma();
		RefreshLeaderHint();
		return card;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnAddLeaderClicked(SCR_ButtonBaseComponent button)
	{
		if (m_aCards.Count() >= CRF_AARReviewData.MAX_LEADERS)
		{
			SetStatus(string.Format("You can rate at most %1 leaders.", CRF_AARReviewData.MAX_LEADERS), true);
			return;
		}

		CRF_AARLeaderCardWidgets card = AddLeaderCard();
		if (card)
			CycleLeader(card, 1); // pre-fill with the first suggested leader
	}

	//------------------------------------------------------------------------------------------------
	protected void OnRemoveLeaderClicked(SCR_ButtonBaseComponent button)
	{
		CRF_AARLeaderCardWidgets card = FindCard(button.GetRootWidget());
		if (!card)
			return;

		m_aCards.RemoveItem(card);
		if (card.m_wRoot)
			card.m_wRoot.RemoveFromHierarchy();

		RefreshKarma();
		RefreshLeaderHint();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnPrevLeaderClicked(SCR_ButtonBaseComponent button)
	{
		CRF_AARLeaderCardWidgets card = FindCard(button.GetRootWidget());
		if (card)
			CycleLeader(card, -1);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnNextLeaderClicked(SCR_ButtonBaseComponent button)
	{
		CRF_AARLeaderCardWidgets card = FindCard(button.GetRootWidget());
		if (card)
			CycleLeader(card, 1);
	}

	//------------------------------------------------------------------------------------------------
	//! Step through this round's leaders (website ORBAT + slotted leaders), skipping names that
	//! another card already rates
	protected void CycleLeader(CRF_AARLeaderCardWidgets card, int direction)
	{
		array<string> options = CRF_AARReviewSession.s_aLeaderOptions;
		if (!options || options.IsEmpty())
		{
			if (!CRF_AARReviewSession.s_bFormReceived)
				SetStatus("Still loading this round's leaders - type a name or try again in a moment.", false);
			else
				SetStatus("No leaders were found for this round - type the leader's name.", false);
			return;
		}

		int count = options.Count();
		int index = card.m_iOptionIndex;
		for (int attempt = 0; attempt < count; attempt++)
		{
			index = (index + direction + count) % count;
			if (!IsNameTaken(options[index], card))
				break;
		}

		card.m_iOptionIndex = index;
		if (card.m_wName)
			card.m_wName.SetText(options[index]);
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsNameTaken(string name, CRF_AARLeaderCardWidgets except)
	{
		foreach (CRF_AARLeaderCardWidgets card : m_aCards)
		{
			if (card != except && card.m_wName && card.m_wName.GetText() == name)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnKarmaUpClicked(SCR_ButtonBaseComponent button)
	{
		SetKarma(FindCard(button.GetRootWidget()), 1);
	}

	//------------------------------------------------------------------------------------------------
	protected void OnKarmaDownClicked(SCR_ButtonBaseComponent button)
	{
		SetKarma(FindCard(button.GetRootWidget()), -1);
	}

	//------------------------------------------------------------------------------------------------
	//! Toggle a vote; like the website, one +1 and one -1 per mission - giving a vote to one leader
	//! takes the same vote away from any other
	protected void SetKarma(CRF_AARLeaderCardWidgets card, int vote)
	{
		if (!card)
			return;

		if (card.m_iKarma == vote)
		{
			card.m_iKarma = 0;
		}
		else
		{
			foreach (CRF_AARLeaderCardWidgets other : m_aCards)
			{
				if (other != card && other.m_iKarma == vote)
					other.m_iKarma = 0;
			}

			card.m_iKarma = vote;
		}

		RefreshKarma();
	}

	//------------------------------------------------------------------------------------------------
	protected void RefreshKarma()
	{
		foreach (CRF_AARLeaderCardWidgets card : m_aCards)
		{
			Widget up = card.m_wRoot.FindAnyWidget("KarmaUpBG");
			if (up)
			{
				if (card.m_iKarma == 1)
					up.SetColor(Color.FromSRGBA(56, 142, 92, 255));
				else
					up.SetColor(Color.FromSRGBA(28, 31, 40, 255));
			}

			Widget down = card.m_wRoot.FindAnyWidget("KarmaDownBG");
			if (down)
			{
				if (card.m_iKarma == -1)
					down.SetColor(Color.FromSRGBA(201, 54, 54, 255));
				else
					down.SetColor(Color.FromSRGBA(28, 31, 40, 255));
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	// Reading / writing the form
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected CRF_AARReviewData Collect()
	{
		CRF_AARReviewData data = new CRF_AARReviewData();
		if (m_wWentWell)
			data.m_sWentWell = m_wWentWell.GetText();
		if (m_wWentBad)
			data.m_sWentBad = m_wWentBad.GetText();
		if (m_wFeedback)
			data.m_sFeedback = m_wFeedback.GetText();

		for (int i = 0; i < CRF_AARReviewData.CATEGORY_COUNT; i++)
			data.m_aCategories[i] = m_aCategoryValues[i];

		foreach (CRF_AARLeaderCardWidgets card : m_aCards)
		{
			CRF_AARLeaderRating leader = new CRF_AARLeaderRating();
			if (card.m_wName)
				leader.m_sName = card.m_wName.GetText();
			if (card.m_wReason)
				leader.m_sReason = card.m_wReason.GetText();
			for (int i = 0; i < CRF_AARReviewData.CATEGORY_COUNT; i++)
				leader.m_aScores[i] = card.m_aScores[i];
			leader.m_iKarma = card.m_iKarma;
			data.m_aLeaders.Insert(leader);
		}

		return data;
	}

	//------------------------------------------------------------------------------------------------
	protected void ApplyReview(CRF_AARReviewData data)
	{
		if (!data)
			return;

		if (m_wWentWell)
			m_wWentWell.SetText(data.m_sWentWell);
		if (m_wWentBad)
			m_wWentBad.SetText(data.m_sWentBad);
		if (m_wFeedback)
			m_wFeedback.SetText(data.m_sFeedback);

		for (int i = 0; i < CRF_AARReviewData.CATEGORY_COUNT; i++)
		{
			m_aCategoryValues[i] = data.m_aCategories[i];
			SetRowValue(m_aCategoryRows[i], m_aCategoryValues[i]);
		}

		foreach (CRF_AARLeaderCardWidgets old : m_aCards)
		{
			if (old.m_wRoot)
				old.m_wRoot.RemoveFromHierarchy();
		}
		m_aCards.Clear();

		foreach (CRF_AARLeaderRating leader : data.m_aLeaders)
		{
			CRF_AARLeaderCardWidgets card = AddLeaderCard();
			if (!card)
				break;

			if (card.m_wName)
				card.m_wName.SetText(leader.m_sName);
			if (card.m_wReason)
				card.m_wReason.SetText(leader.m_sReason);

			card.m_iOptionIndex = CRF_AARReviewSession.s_aLeaderOptions.Find(leader.m_sName);
			for (int i = 0; i < CRF_AARReviewData.CATEGORY_COUNT; i++)
			{
				card.m_aScores[i] = leader.m_aScores[i];
				SetRowValue(card.m_aRows[i], card.m_aScores[i]);
			}

			card.m_iKarma = leader.m_iKarma;
			RefreshCardOverall(card);
		}

		RefreshOverall();
		RefreshKarma();
		RefreshLeaderHint();
	}

	//------------------------------------------------------------------------------------------------
	protected bool IsFormEmpty()
	{
		CRF_AARReviewData data = Collect();
		if (!data.m_sWentWell.IsEmpty() || !data.m_sWentBad.IsEmpty() || !data.m_sFeedback.IsEmpty() || !data.m_aLeaders.IsEmpty())
			return false;

		foreach (int value : data.m_aCategories)
		{
			if (value > 0)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// Server answers
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void OnFormReceived()
	{
		// Load the saved review unless the player already started typing
		if (!m_bAppliedSaved && CRF_AARReviewSession.s_bHasReview && CRF_AARReviewSession.s_Review && IsFormEmpty())
		{
			ApplyReview(CRF_AARReviewSession.s_Review);
			m_bAppliedSaved = true;
		}

		ShowFormStatus();
		RefreshSubmitLabel();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnSubmitResult(bool saved, string message)
	{
		m_bSubmitting = false;

		if (saved)
		{
			CRF_AARReviewSession.s_bHasReview = true;
			CRF_AARReviewSession.s_Review = Collect();
			CRF_AARReviewSession.s_Draft = null;
			SetStatus("Saved to coalitiongroup.net - thanks! You can still edit it while the AAR is open.", false);
		}
		else
		{
			if (message.IsEmpty())
				message = "Your AAR couldn't be saved - try again.";

			SetStatus(message, true);
		}

		RefreshSubmitLabel();
	}

	//------------------------------------------------------------------------------------------------
	// Buttons
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void OnSubmitClicked(SCR_ButtonBaseComponent button)
	{
		if (m_bSubmitting)
			return;

		CRF_AARReviewData data = Collect();
		string error;
		if (!data.Validate(error))
		{
			SetStatus(error, true);
			return;
		}

		COA_PlayerRplToAuthorityManager authorityManager = COA_PlayerRplToAuthorityManager.GetInstance();
		if (!authorityManager)
			return;

		array<string> strings = {};
		array<int> ints = {};
		data.Pack(strings, ints);
		authorityManager.SubmitAARReview(strings, ints);

		m_bSubmitting = true;
		SetStatus("Sending your AAR to coalitiongroup.net...", false);
		RefreshSubmitLabel();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCancelClicked(SCR_ButtonBaseComponent button)
	{
		GetGame().GetCallqueue().CallLater(Close, 0); // not from inside the button's own handler
	}

	//------------------------------------------------------------------------------------------------
	protected void OnBack()
	{
		GetGame().GetCallqueue().CallLater(Close, 0);
	}
}

//------------------------------------------------------------------------------------------------
//! One leader card in CRF_AARReviewMenu
class CRF_AARLeaderCardWidgets
{
	Widget m_wRoot;
	EditBoxWidget m_wName;
	MultilineEditBoxWidget m_wReason;
	TextWidget m_wOverall;
	ref array<Widget> m_aRows = {};
	ref array<int> m_aScores = {0, 0, 0, 0, 0};
	int m_iKarma;
	int m_iOptionIndex = -1;
}
