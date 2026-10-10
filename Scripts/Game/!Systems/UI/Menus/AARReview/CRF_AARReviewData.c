//------------------------------------------------------------------------------------------------
// In-game AAR review (the coalitiongroup.net "Submit Your AAR" form on the AAR screen).
//
// Flow: CRF_AARReviewMenu -> COA_PlayerRplToAuthorityManager.SubmitAARReview -> CRF_AARManager
// (server) -> POST api/game/aar-review on the website, which saves it exactly like the website form.
// Opening the form asks the server for the player's existing review and the leader list
// (GET api/game/aar-review), answered through COA_PlayerRplToOwnerManager.ReceiveAARReviewForm.
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
//! One review. Sent over the network as two flat arrays (see Pack / Unpack):
//!   strings: wentWell, wentBad, feedback, then (leaderName, reason) per leader
//!   ints:    5 mission categories, then (initiative, clarity, organization, adaptability, authority, karma) per leader
class CRF_AARReviewData
{
	static const int CATEGORY_COUNT = 5;
	static const int MAX_LEADERS = 12;
	static const int MAX_TEXT = 4000;
	protected static const int INTS_PER_LEADER = 6;	// 5 scores + karma
	protected static const int STRINGS_PER_LEADER = 2;	// name + reason

	string m_sWentWell;
	string m_sWentBad;
	string m_sFeedback;
	ref array<int> m_aCategories = {0, 0, 0, 0, 0};	// pacing, objectives, terrain, assets, structure
	ref array<ref CRF_AARLeaderRating> m_aLeaders = {};

	//------------------------------------------------------------------------------------------------
	static string GetCategoryLabel(int index)
	{
		switch (index)
		{
			case 0: return "Pacing";
			case 1: return "Objectives";
			case 2: return "Terrain";
			case 3: return "Assets";
			case 4: return "Structure";
		}

		return "";
	}

	//------------------------------------------------------------------------------------------------
	static string GetLeaderCategoryLabel(int index)
	{
		switch (index)
		{
			case 0: return "Initiative";
			case 1: return "Clarity";
			case 2: return "Organization";
			case 3: return "Adaptability";
			case 4: return "Authority";
		}

		return "";
	}

	//------------------------------------------------------------------------------------------------
	//! Average of the rated categories, rounded like the website (0 when nothing is rated)
	static int Average(array<int> values)
	{
		int total, count;
		foreach (int value : values)
		{
			if (value >= 1)
			{
				total += value;
				count++;
			}
		}

		if (count == 0)
			return 0;

		float average = total;
		average /= count;
		return Math.Round(average);
	}

	//------------------------------------------------------------------------------------------------
	//! The website's rules, checked before sending (client) and before posting (server)
	bool Validate(out string error)
	{
		foreach (int value : m_aCategories)
		{
			if (value < 1 || value > 5)
			{
				error = "Rate all five mission categories before submitting.";
				return false;
			}
		}

		if (m_aLeaders.Count() > MAX_LEADERS)
		{
			error = string.Format("You can rate at most %1 leaders.", MAX_LEADERS);
			return false;
		}

		int positiveKarma, negativeKarma;
		foreach (CRF_AARLeaderRating leader : m_aLeaders)
		{
			string name = leader.m_sName.Trim();
			if (name.IsEmpty())
			{
				error = "Every leader card needs a name - fill it in or remove the card.";
				return false;
			}

			foreach (int score : leader.m_aScores)
			{
				if (score < 1 || score > 5)
				{
					error = string.Format("Rate all five leadership categories for %1.", name);
					return false;
				}
			}

			if (leader.m_iKarma == 1)
				positiveKarma++;
			else if (leader.m_iKarma == -1)
				negativeKarma++;
			else if (leader.m_iKarma != 0)
			{
				error = "Karma can only be +1 or -1.";
				return false;
			}
		}

		if (positiveKarma > 1 || negativeKarma > 1)
		{
			error = "Only one +1 and one -1 karma vote are allowed per mission.";
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	void Pack(notnull array<string> outStrings, notnull array<int> outInts)
	{
		outStrings.Clear();
		outInts.Clear();
		outStrings.Insert(Clip(m_sWentWell));
		outStrings.Insert(Clip(m_sWentBad));
		outStrings.Insert(Clip(m_sFeedback));
		for (int i = 0; i < CATEGORY_COUNT; i++)
			outInts.Insert(m_aCategories[i]);

		foreach (CRF_AARLeaderRating leader : m_aLeaders)
		{
			outStrings.Insert(leader.m_sName.Trim());
			outStrings.Insert(Clip(leader.m_sReason));
			for (int i = 0; i < CATEGORY_COUNT; i++)
				outInts.Insert(leader.m_aScores[i]);
			outInts.Insert(leader.m_iKarma);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return null if the arrays don't describe a review
	static CRF_AARReviewData Unpack(array<string> strings, array<int> ints)
	{
		if (!strings || !ints || strings.Count() < 3 || ints.Count() < CATEGORY_COUNT)
			return null;

		int leaderCount = (strings.Count() - 3) / STRINGS_PER_LEADER;
		if (leaderCount > MAX_LEADERS
			|| strings.Count() != 3 + leaderCount * STRINGS_PER_LEADER
			|| ints.Count() != CATEGORY_COUNT + leaderCount * INTS_PER_LEADER)
			return null;

		CRF_AARReviewData data = new CRF_AARReviewData();
		data.m_sWentWell = Clip(strings[0]);
		data.m_sWentBad = Clip(strings[1]);
		data.m_sFeedback = Clip(strings[2]);
		for (int i = 0; i < CATEGORY_COUNT; i++)
			data.m_aCategories[i] = ints[i];

		for (int l = 0; l < leaderCount; l++)
		{
			CRF_AARLeaderRating leader = new CRF_AARLeaderRating();
			leader.m_sName = strings[3 + l * STRINGS_PER_LEADER];
			leader.m_sReason = Clip(strings[4 + l * STRINGS_PER_LEADER]);
			int offset = CATEGORY_COUNT + l * INTS_PER_LEADER;
			for (int i = 0; i < CATEGORY_COUNT; i++)
				leader.m_aScores[i] = ints[offset + i];
			leader.m_iKarma = ints[offset + CATEGORY_COUNT];
			data.m_aLeaders.Insert(leader);
		}

		return data;
	}

	//------------------------------------------------------------------------------------------------
	static string Clip(string text)
	{
		if (text.Length() > MAX_TEXT)
			return text.Substring(0, MAX_TEXT);

		return text;
	}
}

//------------------------------------------------------------------------------------------------
class CRF_AARLeaderRating
{
	string m_sName;
	string m_sReason;
	ref array<int> m_aScores = {0, 0, 0, 0, 0};	// initiative, clarity, organization, adaptability, authority
	int m_iKarma;										// -1, 0, +1
}

//------------------------------------------------------------------------------------------------
//! Client: this round's review state, kept while the AAR screen is open and reopened
class CRF_AARReviewSession
{
	static int s_iMissionId;				// the round this state belongs to
	static bool s_bFormRequested;
	static bool s_bFormReceived;
	static bool s_bLinked = true;			// player's Arma GUID is linked to a website account
	static bool s_bHasReview;				// a review is saved on the website
	static ref CRF_AARReviewData s_Review;	// the saved review (when s_bHasReview)
	static ref CRF_AARReviewData s_Draft;	// unsent edits from the last time the form was closed
	static ref array<string> s_aLeaderOptions = {};

	static ref ScriptInvoker s_OnFormReceived = new ScriptInvoker();
	static ref ScriptInvoker s_OnSubmitResult = new ScriptInvoker();	// (bool saved, string message)

	//------------------------------------------------------------------------------------------------
	//! Forget a previous round's state
	static void SyncMission()
	{
		if (s_iMissionId == CRF_AARSessionStats.s_iMissionId)
			return;

		s_iMissionId = CRF_AARSessionStats.s_iMissionId;
		s_bFormRequested = false;
		s_bFormReceived = false;
		s_bLinked = true;
		s_bHasReview = false;
		s_Review = null;
		s_Draft = null;
		s_aLeaderOptions.Clear();
	}

	//------------------------------------------------------------------------------------------------
	static void ReceiveForm(bool linked, bool hasReview, array<string> strings, array<int> ints, array<string> leaderOptions)
	{
		SyncMission();
		s_bFormReceived = true;
		s_bLinked = linked;
		s_bHasReview = hasReview;
		if (hasReview)
			s_Review = CRF_AARReviewData.Unpack(strings, ints);

		s_aLeaderOptions.Clear();
		if (leaderOptions)
		{
			foreach (string option : leaderOptions)
				s_aLeaderOptions.Insert(option);
		}

		s_OnFormReceived.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	static void ReceiveResult(bool saved, string message)
	{
		s_OnSubmitResult.Invoke(saved, message);
	}
}

//------------------------------------------------------------------------------------------------
// GET api/game/aar-review response, read with JsonLoadContext (field names match the JSON keys)
class CRF_AARReviewFormJson
{
	bool success;
	bool linked;
	bool hasReview;
	ref CRF_AARReviewJson review = new CRF_AARReviewJson();
	ref array<string> leaderOptions = {};
}

//------------------------------------------------------------------------------------------------
class CRF_AARReviewJson
{
	string went_well;
	string went_bad;
	string mission_feedback;
	int pacing;
	int objectives_rating;
	int terrain_rating;
	int assets_rating;
	int structure_rating;
	ref array<ref CRF_AARLeaderJson> leadershipRatings = {};
}

//------------------------------------------------------------------------------------------------
class CRF_AARLeaderJson
{
	string leader_name;
	int initiative;
	int clarity;
	int organization;
	int adaptability;
	int authority;
	int karma_vote;
	string reason;
}
