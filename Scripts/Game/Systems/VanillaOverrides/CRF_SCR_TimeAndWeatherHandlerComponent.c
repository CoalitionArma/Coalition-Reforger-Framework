modded class SCR_TimeAndWeatherHandlerComponent
{
	bool GetRandomStartingWeather()
	{
		return m_bRandomStartingWeather;
	}
	
	bool GetRandomWeatherChanges()
	{
		return m_bRandomWeatherChanges;
	}
	
	array<ref SCR_TimeAndWeatherState> GetStartingWeatherAndTime()
	{
		return m_aStartingWeatherAndTime;
	}
}