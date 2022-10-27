#include "pch.h"
#include "IWibbleWobbleCapture.h"
#include "WibbleWobbleCapture.h"
#include "WWFreetrack.h"
#include "WWSharedMemory.h"
#include "Debug.h"

static WWSharedMemory* s_captureDataIPC = NULL;
static WWCaptureData s_captureData;
static WWFreetrack s_wwFreeTrack;
static int s_frameCounter = 0;
static WWTrackingConfig s_wwTrackingConfig;
static WWSharedMemory* s_wwTrackingConfigIPC;


bool ValidateHeadTracking()
{
	if (s_captureDataIPC == NULL)
	{
		s_captureDataIPC = new WWSharedMemory(s_captureDataName, s_captureDataMutexName, WWSharedMemType::WWSharedMem_ReadWrite, &s_captureData, sizeof(s_captureData));
	}

	if (s_wwTrackingConfigIPC == NULL)
	{
		s_wwTrackingConfigIPC = new WWSharedMemory(s_trackingConfigName, s_trackingConfigMutexName, WWSharedMemType::WWSharedMem_ReadWrite, &s_wwTrackingConfig, sizeof(s_wwTrackingConfig));
	}


	if (!s_wwFreeTrack.IsInitialized())
	{
		if (!s_wwFreeTrack.Create())
			Debug::Log("Freetrack Initialized\n");
		else
			Debug::Log("Freetrack Initialize failed\n");
	}


	return true;
}

__declspec(dllexport) bool WWCleanupHeadTracking()
{
	if (s_captureDataIPC != NULL)
	{
		s_captureDataIPC->Destroy();
		delete(s_captureDataIPC);
		s_captureDataIPC = NULL;
	}

	if (s_wwTrackingConfigIPC != NULL)
	{
		s_wwTrackingConfigIPC->Destroy();
		delete(s_wwTrackingConfigIPC);
		s_wwTrackingConfigIPC = NULL;
	}
	

	if (s_wwFreeTrack.IsInitialized())
	{
		s_wwFreeTrack.Destroy();

	}

	return true;
}

__declspec(dllexport) bool WWTickHeadTracking(WWCaptureHeadTrackData &a_dataOut)
{
	if (!ValidateHeadTracking())
	{
		return false;
	}

	//insert head tracking config
	s_wwTrackingConfigIPC->Read();
	a_dataOut.m_vFov = s_wwTrackingConfig.m_vFov;
	a_dataOut.m_hFov = s_wwTrackingConfig.m_hFov;
	a_dataOut.m_worldScale = s_wwTrackingConfig.m_worldScale;

	//Read freetrack
	WWFreetrack::FTData* ftData = s_wwFreeTrack.GetFTData();
	if (ftData != NULL)
	{
		a_dataOut.m_x = ftData->X;
		a_dataOut.m_y = ftData->Y;
		a_dataOut.m_z = ftData->Z;

		a_dataOut.m_pitch = ftData->Pitch;
		a_dataOut.m_yaw = ftData->Yaw;
		a_dataOut.m_roll = ftData->Roll;
	}
	else
	{
		Debug::Log("NULL ftData\n");
		return true;
	}

	//read latest capture data state
	s_captureDataIPC->LockMutex();
	s_captureDataIPC->ReadUnsafe();

	//server initializing, do nothing
	if (!(s_captureData.m_state == WWCaptureDataState_ServingCPU || s_captureData.m_state == WWCaptureDataState_ServingOnPresent))
	{
		s_captureDataIPC->UnlockMutex();
		return false;
	}

	//notify system we are serving head tracking
	if (s_captureData.m_state != WWCaptureDataState_ServingCPU)
	{
		s_captureData.m_state = WWCaptureDataState_ServingCPU;
		Debug::Log("Set CaptureState as ServingCPU\n");
	}

	//find oldest frame
	int oldestFrameCount = INT_MAX;
	WWCaptureFrame* bestFrame = NULL;
	for (int i = 0; i < WW_CAPTURE_FRAME_COUNT; ++i)
	{
		//Debug::Log("Server CPU: FrameCounter: %d\n", s_captureData.m_frames[i].m_framecounter);
		//Debug::Log("Server CPU: Read texture resource: %x\n", s_captureData.m_frames[i].m_textureResource);
		//Debug::Log("Server CPU: Read state: %d\n", s_captureData.m_frames[i].m_state);
		
		//pick oldest frame that is ready for serving a head tracking frame in the cpu thread
		if (s_captureData.m_frames[i].m_framecounter < oldestFrameCount && s_captureData.m_frames[i].m_state == WWCaptureFrameState_ReadyForServerCPU)
		{
			oldestFrameCount = s_captureData.m_frames[i].m_framecounter;
			bestFrame = &s_captureData.m_frames[i];
		}
	}

	if (bestFrame != NULL)
	{
		//Debug::Log("Server CPU: NEW FrameCounter: %d\n", a_frame.m_framecounter);
		//Debug::Log("Server CPU: Oldest frame counter: %d\n", m_freeTrackFrame.m_frames[oldestFrameIdx].m_framecounter);
		//Debug::Log("Server CPU: Oldest frame state: %d\n", m_freeTrackFrame.m_frames[oldestFrameIdx].m_state);

		bestFrame->m_framecounter = s_frameCounter++;
		bestFrame->m_state = WWCaptureFrameState_ReadyForServerGPUCopy;
		bestFrame->m_headTrackData = a_dataOut;
	}

	//write back capture data state
	s_captureDataIPC->WriteUnsafe();
	s_captureDataIPC->UnlockMutex();

	return true;

}