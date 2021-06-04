#pragma once
#include <utility/Utility.h>

namespace GEE
{
	enum ShadingModel
	{
		SHADING_FULL_LIT,
		SHADING_PHONG,
		SHADING_PBR_COOK_TORRANCE
	};

	enum AntiAliasingType
	{
		AA_NONE,
		AA_SMAA1X,
		AA_SMAAT2X
	};

	enum ToneMappingType
	{
		TM_NONE,
		TM_REINHARD
	};

	enum SettingLevel
	{
		SETTING_NONE,
		SETTING_LOW,
		SETTING_MEDIUM,
		SETTING_HIGH,
		SETTING_ULTRA
	};

	struct GameSettings
	{
		Vec2u WindowSize;

		bool bWindowFullscreen;
		std::string WindowTitle;

		struct VideoSettings
		{
			Vec2f Resolution;
			ShadingModel Shading;

			unsigned int AmbientOcclusionSamples;
			bool bVSync;
			bool bBloom;
			bool DrawToWindowFBO;
			AntiAliasingType AAType;
			SettingLevel AALevel;
			SettingLevel POMLevel;
			SettingLevel ShadowLevel;
			ToneMappingType TMType;

			float MonitorGamma;

			VideoSettings();
			bool IsVelocityBufferNeeded() const;
			bool IsTemporalReprojectionEnabled() const;
			std::string GetShaderDefines(Vec2u resolution = Vec2u(0)) const;
			virtual bool LoadSetting(std::stringstream& filestr, std::string settingName);
		} Video;

		/////////////////////////////

		GameSettings();
		GameSettings(std::string path);

		void LoadFromFile(std::string path);

		virtual bool LoadSetting(std::stringstream& filestr, std::string settingName);
	};


	struct Dupa : public GameSettings
	{
		struct LOL : public GameSettings::VideoSettings
		{

		} Video;
	};

	template <class T> void LoadEnum(std::stringstream& filestr, T& var);
}