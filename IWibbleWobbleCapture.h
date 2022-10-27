#pragma once

static const float rad2deg = (180.0f / 3.14159265359f);
static const float deg2rad = (3.14159265359f / 180.0f);


#pragma pack( push, 0 )
volatile struct __declspec(dllexport) WWCaptureHeadTrackData
{
	int version = 0; // version
	float m_yaw = 0;   // positive yaw to the left
	float m_pitch = 0; // positive pitch up 
	float m_roll = 0;  // positive roll to the left 
	float m_x = 0; // local position x
	float m_y = 0; // local position y
	float m_z = 0; // local position z
	float m_vFov = 120; // vertical field of view
	float m_hFov = 120; // horizontal field of view
	float m_worldScale = 1; // world scale multiplier

};
#pragma pack(pop)


__declspec(dllexport) bool WWTickHeadTracking(WWCaptureHeadTrackData& a_dataOut);
__declspec(dllexport) bool WWCleanupHeadTracking();




