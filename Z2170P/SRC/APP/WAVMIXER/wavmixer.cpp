// All rights reserved ADENEO EMBEDDED 2010
// WaveMixer.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <stdio.h>

#define NELEMS(a)			(sizeof(a)/sizeof((a)[0]))

PTSTR usage_text[] =
{
    TEXT("usage:\r\n"),
    TEXT("       -l <IN|OUT>						List all available controls for WaveIN or WavOUT device\r\n"),
    TEXT("       -s <IN|OUT> <Control ID> <Value>	Pass a value to set for a particular control\r\n"),
    TEXT("       -?									Display help\r\n"),
};

void Usage(void)
{ 
	int i;

    for (i = 0; i < NELEMS(usage_text); i++) 
	{
		_tprintf(usage_text[i]);
    }
}

BOOL ListMixerControls(BOOL bIn)
{
	BOOL				bRet = FALSE;
	MMRESULT			mr = MMSYSERR_NOERROR;
	HMIXER				hMixer = NULL;
	HWAVEIN				hWaveIn = NULL;
	DWORD				dwMixerNumDevs = 0;
	HANDLE				hevDone = NULL;
    WAVEFORMATEX		wfx;
	MIXERCONTROLDETAILS sMixerControlDetails;
	MIXERLINECONTROLS	sMixerLineControls;
	MIXERCONTROL*		pMixerControls = NULL;
	MIXERCONTROLDETAILS_LISTTEXT* pMixerControlListText = NULL;
	MIXERCONTROLDETAILS_BOOLEAN* pMixerControlDetailsBoolean = NULL;
	MIXERCONTROLDETAILS_UNSIGNED* pMixerControlDetailsUnsigned = NULL;
	MIXERLINE			sMixerLine;

    wfx.cbSize = 0;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.wBitsPerSample = 16;
    wfx.nSamplesPerSec = 44100;
    wfx.nChannels = 1;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

    hevDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hevDone == NULL) 
	{
        return 0;
    }

	dwMixerNumDevs = mixerGetNumDevs();
	if (dwMixerNumDevs == 0)
	{
		_tprintf(_T("ERROR : No wave device available on this platform\n"));
		goto cleanUp;
	}
	
	mr = waveInOpen(&hWaveIn, 0, &wfx, (DWORD)hevDone, 0, CALLBACK_EVENT);
	if (mr != MMSYSERR_NOERROR)
	{
		_tprintf(_T("ERROR : Could not open Wave IN device\n"));
		goto cleanUp;
	}

	mr = mixerOpen(&hMixer, (UINT)hWaveIn, (DWORD)hevDone, 0, MIXER_OBJECTF_HWAVEIN);
	if (mr != MMSYSERR_NOERROR)
	{
		_tprintf(_T("ERROR : Could not get an handle on the mixer device\n"));
		goto cleanUp;
	}

	// Initialize Mixer Line Structure
	memset(&sMixerLine, 0, sizeof(sMixerLine));
	sMixerLine.cbStruct = sizeof(sMixerLine);
	sMixerLine.dwComponentType = bIn ? MIXERLINE_COMPONENTTYPE_DST_WAVEIN : MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

	mixerGetLineInfo((HMIXEROBJ)hMixer, &sMixerLine, MIXER_OBJECTF_HMIXER|MIXER_GETLINEINFOF_COMPONENTTYPE);
	if (mr != MMSYSERR_NOERROR)
	{
		_tprintf(_T("ERROR : Could not get line info\n"));
		goto cleanUp;
	}

	// Allocate memory to store the Mixer Controls
	pMixerControls = (MIXERCONTROL*)LocalAlloc(LPTR, sMixerLine.cControls * sizeof(MIXERCONTROL));
	if (pMixerControls == NULL)
	{
		_tprintf(_T("ERROR : Could not allocate memory\n"));
		goto cleanUp;
	}

	// Initialize Mixer Line Controls Structure
	memset(&sMixerLineControls, 0, sizeof(sMixerLineControls));
	sMixerLineControls.dwLineID = sMixerLine.dwLineID;
	sMixerLineControls.cbStruct = sizeof(sMixerLineControls);
	sMixerLineControls.cControls = sMixerLine.cControls;
	sMixerLineControls.cbmxctrl = sizeof(MIXERCONTROL);
	sMixerLineControls.pamxctrl = pMixerControls;

	mr = mixerGetLineControls((HMIXEROBJ)hMixer, &sMixerLineControls, MIXER_OBJECTF_HMIXER|MIXER_GETLINECONTROLSF_ALL);
	if (mr != MMSYSERR_NOERROR)
	{
		_tprintf(_T("ERROR : Could not get line controls\n"));
		goto cleanUp;
	}

	_tprintf(_T("%s : %s\r\n"), bIn ? _T("WAVE IN") : _T("WAVE OUT"), sMixerLine.szName);
	for (DWORD i=0; i<sMixerLineControls.cControls;i++)
	{
		// Display control items if any
		if (sMixerLineControls.pamxctrl[i].cMultipleItems > 1)
		{
			wchar_t indexStr[16];
			wchar_t valStr[16];
			_itow((int)i, indexStr, 10);
			_tprintf(_T("\t%s: %s\r\n"), indexStr, sMixerLineControls.pamxctrl[i].szName);

			DWORD dwNumItems = sMixerLineControls.pamxctrl[i].cMultipleItems;

			if (pMixerControlListText != NULL)
			{
				LocalFree(pMixerControlListText);
				pMixerControlListText = NULL;
			}

			pMixerControlListText = (MIXERCONTROLDETAILS_LISTTEXT*)LocalAlloc(LPTR,
				dwNumItems * sizeof(MIXERCONTROLDETAILS_LISTTEXT));
			if (pMixerControlListText == NULL)
			{
				_tprintf(_T("ERROR : Could not allocate memory\n"));
				goto cleanUp;
			}

			// Initialize Mixer Details Structure
			memset(&sMixerControlDetails, 0, sizeof(sMixerControlDetails));
			sMixerControlDetails.dwControlID = sMixerLineControls.pamxctrl[i].dwControlID;
			sMixerControlDetails.cbStruct = sizeof(sMixerControlDetails);
			sMixerControlDetails.cChannels = sMixerLine.cChannels;
			sMixerControlDetails.cMultipleItems = sMixerLineControls.pamxctrl[i].cMultipleItems;
			sMixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
			sMixerControlDetails.paDetails = (LPVOID)pMixerControlListText;

			// Get control items names
			mr = mixerGetControlDetails((HMIXEROBJ)hMixer, &sMixerControlDetails, MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_LISTTEXT );
			if (mr != MMSYSERR_NOERROR)
			{
				_tprintf(_T("ERROR : Could not get control details\n"));
				goto cleanUp;
			}

			// Get control items values

			// Initialize Mixer Details Structure
			switch(sMixerLineControls.pamxctrl[i].dwControlType)
			{
				case MIXERCONTROL_CONTROLTYPE_MUX:
					
					if (pMixerControlDetailsBoolean != NULL)
					{
						LocalFree(pMixerControlDetailsBoolean);
						pMixerControlDetailsBoolean = NULL;
					}

					pMixerControlDetailsBoolean = (MIXERCONTROLDETAILS_BOOLEAN*)LocalAlloc(LPTR,
						sMixerControlDetails.cChannels      *
						sMixerControlDetails.cMultipleItems * 
						sizeof(MIXERCONTROLDETAILS_BOOLEAN));
					
					if (pMixerControlDetailsBoolean == NULL)
					{
						_tprintf(_T("ERROR : Failed to allocate memory\n"));
						goto cleanUp;
					}
					
					sMixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
					sMixerControlDetails.paDetails = (LPVOID)pMixerControlDetailsBoolean;
					break;
				default:
					_tprintf(_T("ERROR : Unsupported control type\n"));
					goto cleanUp;
			}

			mr = mixerGetControlDetails((HMIXEROBJ)hMixer, &sMixerControlDetails, MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE );
			if (mr != MMSYSERR_NOERROR)
			{
				_tprintf(_T("ERROR : Could not get control details\n"));
				goto cleanUp;
			}

			// Display all control items
			for (DWORD j=0; j<dwNumItems; j++)
			{
				_itow((int)j, indexStr, 10);
				wcscpy_s(valStr, 16, pMixerControlDetailsBoolean[j].fValue ? TEXT("TRUE") : TEXT("FALSE"));
				_tprintf(_T("\t\t%s: %s = %s\r\n"), 
					indexStr, 
					pMixerControlListText[j].szName,
					valStr);
			}

			// Free allocated memory
			LocalFree(pMixerControlListText);
			pMixerControlListText = NULL;
		}
		else // Otherwise display item and its value
		{
			wchar_t indexStr[16];
			wchar_t valStr[64];
			_itow((int)i, indexStr, 10);

			// Get control value

			// Initialize Mixer Details Structure
			memset(&sMixerControlDetails, 0, sizeof(sMixerControlDetails));
			sMixerControlDetails.dwControlID = sMixerLineControls.pamxctrl[i].dwControlID;
			sMixerControlDetails.cbStruct = sizeof(sMixerControlDetails);
			sMixerControlDetails.cChannels = sMixerLine.cChannels;
			sMixerControlDetails.cMultipleItems = 1;

			switch(sMixerLineControls.pamxctrl[i].dwControlType)
			{
				case MIXERCONTROL_CONTROLTYPE_BOOLEAN:
				case MIXERCONTROL_CONTROLTYPE_MUTE:
				case MIXERCONTROL_CONTROLTYPE_MUX:
					if (pMixerControlDetailsBoolean != NULL)
					{
						LocalFree(pMixerControlDetailsBoolean);
						pMixerControlDetailsBoolean = NULL;
					}

					pMixerControlDetailsBoolean = (MIXERCONTROLDETAILS_BOOLEAN*)LocalAlloc(LPTR,
						sMixerControlDetails.cChannels      *
						sizeof(MIXERCONTROLDETAILS_BOOLEAN));
					
					if (pMixerControlDetailsBoolean == NULL)
					{
						_tprintf(_T("ERROR : Failed to allocate memory\n"));
						goto cleanUp;
					}
					
					sMixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
					sMixerControlDetails.paDetails = (LPVOID)pMixerControlDetailsBoolean;
					break;
				case MIXERCONTROL_CONTROLTYPE_VOLUME:
					
					if (pMixerControlDetailsUnsigned != NULL)
					{
						LocalFree(pMixerControlDetailsUnsigned);
						pMixerControlDetailsUnsigned = NULL;
					}

					pMixerControlDetailsUnsigned = (MIXERCONTROLDETAILS_UNSIGNED*)LocalAlloc(LPTR,
						sMixerControlDetails.cChannels      *
						sizeof(MIXERCONTROLDETAILS_UNSIGNED));

					if (pMixerControlDetailsUnsigned == NULL)
					{
						_tprintf(_T("ERROR : Failed to allocate memory\n"));
						goto cleanUp;
					}
					
					sMixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
					sMixerControlDetails.paDetails = (LPVOID)pMixerControlDetailsUnsigned;
					break;
				default:
					_tprintf(_T("ERROR : Unsupported control type\n"));
					goto cleanUp;
			}

			mr = mixerGetControlDetails((HMIXEROBJ)hMixer, &sMixerControlDetails, MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE );
			if (mr != MMSYSERR_NOERROR)
			{
				_tprintf(_T("ERROR : Could not get control details\n"));
				goto cleanUp;
			}

			switch(sMixerLineControls.pamxctrl[i].dwControlType)
			{
				case MIXERCONTROL_CONTROLTYPE_BOOLEAN:
				case MIXERCONTROL_CONTROLTYPE_MUTE:
				case MIXERCONTROL_CONTROLTYPE_MUX:
					wcscpy_s(valStr, 16, pMixerControlDetailsBoolean[0].fValue ? TEXT("TRUE") : TEXT("FALSE"));
					break;
				case MIXERCONTROL_CONTROLTYPE_VOLUME:
					wchar_t tmpStr[16];
					if (sMixerControlDetails.cChannels == 2)
					{
						_itow(pMixerControlDetailsUnsigned[0].dwValue, tmpStr, 16);
						wcscpy_s(valStr, 16, TEXT("L 0x"));
						wcsncat(valStr, tmpStr, 16);
						_itow(pMixerControlDetailsUnsigned[1].dwValue, tmpStr, 16);
						wcsncat(valStr, TEXT(" | R 0x"), 16);
						wcsncat(valStr, tmpStr, 16);
					}
					else if (sMixerControlDetails.cChannels == 1)
					{
						_itow(pMixerControlDetailsUnsigned[0].dwValue, tmpStr, 16);
						wcscpy_s(valStr, 16, TEXT("0x"));
						wcsncat(valStr, tmpStr, 16);
					}
					else
					{
						wcscpy_s(valStr, 16, TEXT("Wrong number of channel"));
					}
					break;
				default:
					_itow(-1, valStr, 16);
					break;
			}

			_tprintf(_T("\t%s: %s = %s\r\n"), 
				indexStr, 
				sMixerLineControls.pamxctrl[i].szName,
				valStr);
		}
	}

	bRet = TRUE;

cleanUp:

	if (pMixerControlDetailsUnsigned != NULL)
	{
		LocalFree(pMixerControlDetailsUnsigned);
	}

	if (pMixerControlDetailsBoolean != NULL)
	{
		LocalFree(pMixerControlDetailsBoolean);
	}

	if (pMixerControlListText != NULL)
	{
		LocalFree(pMixerControlListText);
	}

	if (pMixerControls != NULL)
	{
		LocalFree(pMixerControls);
	}

	if (hMixer != NULL)
	{
		mixerClose(hMixer);
	}

	if (hWaveIn != NULL)
	{
		waveInClose(hWaveIn);
	}

    if (hevDone != NULL) 
	{
        CloseHandle(hevDone);
    }

    return bRet;
}

BOOL SetControlValue(BOOL bIn, LONG controlID, LONG value)
{
	BOOL				bRet = FALSE;
	MMRESULT			mr = MMSYSERR_NOERROR;
	HMIXER				hMixer = NULL;
	HWAVEIN				hWaveIn = NULL;
	DWORD				dwMixerNumDevs = 0;
	HANDLE				hevDone = NULL;
    WAVEFORMATEX		wfx;
	MIXERLINECONTROLS	sMixerLineControls;
	MIXERLINE			sMixerLine;
	MIXERCONTROLDETAILS sMixerControlDetails;
	MIXERCONTROL*		pMixerControls = NULL;
	MIXERCONTROLDETAILS_BOOLEAN* pMixerControlDetailsBoolean = NULL;
	MIXERCONTROLDETAILS_UNSIGNED* pMixerControlDetailsUnsigned = NULL;

    wfx.cbSize = 0;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.wBitsPerSample = 16;
    wfx.nSamplesPerSec = 44100;
    wfx.nChannels = 1;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

    hevDone = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hevDone == NULL) 
	{
        return 0;
    }

	dwMixerNumDevs = mixerGetNumDevs();
	if (dwMixerNumDevs == 0)
	{
		_tprintf(_T("ERROR : No wave device available on this platform\n"));
		goto cleanUp;
	}
	
	mr = waveInOpen(&hWaveIn, 0, &wfx, (DWORD)hevDone, 0, CALLBACK_EVENT);
	if (mr != MMSYSERR_NOERROR)
	{
		_tprintf(_T("ERROR : Could not open Wave IN device\n"));
		goto cleanUp;
	}

	mr = mixerOpen(&hMixer, (UINT)hWaveIn, (DWORD)hevDone, 0, MIXER_OBJECTF_HWAVEIN);
	if (mr != MMSYSERR_NOERROR)
	{
		_tprintf(_T("ERROR : Could not get an handle on the mixer device\n"));
		goto cleanUp;
	}

	// Initialize Mixer Line Structure
	memset(&sMixerLine, 0, sizeof(sMixerLine));
	sMixerLine.cbStruct = sizeof(sMixerLine);
	sMixerLine.dwComponentType = bIn ? MIXERLINE_COMPONENTTYPE_DST_WAVEIN : MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

	mixerGetLineInfo((HMIXEROBJ)hMixer, &sMixerLine, MIXER_OBJECTF_HMIXER|MIXER_GETLINEINFOF_COMPONENTTYPE);
	if (mr != MMSYSERR_NOERROR)
	{
		_tprintf(_T("ERROR : Could not get line info\n"));
		goto cleanUp;
	}

	// Check that control ID is valid
	if ((DWORD)controlID >= sMixerLine.cControls)
	{
		_tprintf(_T("ERROR : Invalid control ID\n"));
		goto cleanUp;
	}

	// Allocate memory to store the Mixer Controls
	pMixerControls = (MIXERCONTROL*)LocalAlloc(LPTR, sMixerLine.cControls * sizeof(MIXERCONTROL));
	if (pMixerControls == NULL)
	{
		_tprintf(_T("ERROR : Could not allocate memory\n"));
		goto cleanUp;
	}

	// Initialize Mixer Line Controls Structure
	memset(&sMixerLineControls, 0, sizeof(sMixerLineControls));
	sMixerLineControls.dwLineID = sMixerLine.dwLineID;
	sMixerLineControls.cbStruct = sizeof(sMixerLineControls);
	sMixerLineControls.cControls = sMixerLine.cControls;
	sMixerLineControls.cbmxctrl = sizeof(MIXERCONTROL);
	sMixerLineControls.pamxctrl = pMixerControls;

	mr = mixerGetLineControls((HMIXEROBJ)hMixer, &sMixerLineControls, MIXER_OBJECTF_HMIXER|MIXER_GETLINECONTROLSF_ALL);
	if (mr != MMSYSERR_NOERROR)
	{
		_tprintf(_T("ERROR : Could not get line controls\n"));
		goto cleanUp;
	}
	
	// Initialize Mixer Details Structure
	memset(&sMixerControlDetails, 0, sizeof(sMixerControlDetails));
	sMixerControlDetails.dwControlID = pMixerControls[controlID].dwControlID;
	sMixerControlDetails.cbStruct = sizeof(sMixerControlDetails);
	sMixerControlDetails.cChannels = sMixerLine.cChannels;
	sMixerControlDetails.cMultipleItems = pMixerControls[controlID].cMultipleItems;

	switch(pMixerControls[controlID].dwControlType)
	{
		case MIXERCONTROL_CONTROLTYPE_MUTE:
		case MIXERCONTROL_CONTROLTYPE_BOOLEAN:
		case MIXERCONTROL_CONTROLTYPE_MUX:

			if (sMixerControlDetails.cMultipleItems > 0)
			{
				pMixerControlDetailsBoolean = (MIXERCONTROLDETAILS_BOOLEAN*)LocalAlloc(LPTR,
					sMixerControlDetails.cChannels		*
					sMixerControlDetails.cMultipleItems	*
					sizeof(MIXERCONTROLDETAILS_BOOLEAN));
			}
			else
			{
				pMixerControlDetailsBoolean = (MIXERCONTROLDETAILS_BOOLEAN*)LocalAlloc(LPTR,
					sMixerControlDetails.cChannels		*
					sizeof(MIXERCONTROLDETAILS_BOOLEAN));
			}
			
			if (pMixerControlDetailsBoolean == NULL)
			{
				_tprintf(_T("ERROR : Failed to allocate memory\n"));
				goto cleanUp;
			}
			
			sMixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
			sMixerControlDetails.paDetails = (LPVOID)pMixerControlDetailsBoolean;
			break;
		case MIXERCONTROL_CONTROLTYPE_VOLUME:

			pMixerControlDetailsUnsigned = (MIXERCONTROLDETAILS_UNSIGNED*)LocalAlloc(LPTR,
					sMixerControlDetails.cChannels		*
					sizeof(MIXERCONTROLDETAILS_UNSIGNED));
			
			if (pMixerControlDetailsUnsigned == NULL)
			{
				_tprintf(_T("ERROR : Failed to allocate memory\n"));
				goto cleanUp;
			}
			
			sMixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
			sMixerControlDetails.paDetails = (LPVOID)pMixerControlDetailsUnsigned;
			break;
		default:
			_tprintf(_T("ERROR : Unsupported control type\n"));
			goto cleanUp;
	}

	mr = mixerGetControlDetails((HMIXEROBJ)hMixer, &sMixerControlDetails, MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE);
	if (mr != MMSYSERR_NOERROR)
	{
		_tprintf(_T("ERROR : Could not get control details\n"));
		goto cleanUp;
	}

	// Change value
	switch(pMixerControls[controlID].dwControlType)
	{
		case MIXERCONTROL_CONTROLTYPE_MUX:
			if (value > 0)
			{
				pMixerControlDetailsBoolean[0].fValue = 0;
				pMixerControlDetailsBoolean[1].fValue = 1;
			}
			else
			{
				pMixerControlDetailsBoolean[0].fValue = 1;
				pMixerControlDetailsBoolean[1].fValue = 0;
			}
			break;
		case MIXERCONTROL_CONTROLTYPE_VOLUME:

			if (sMixerControlDetails.cChannels == 2)
			{
				pMixerControlDetailsUnsigned[0].dwValue = (value >> 16) & 0xFFFF;
				pMixerControlDetailsUnsigned[1].dwValue = (value >>  0) & 0xFFFF;
			}
			else if (sMixerControlDetails.cChannels == 1)
			{
				pMixerControlDetailsUnsigned[0].dwValue = value & 0xFFFF;
			}
			break;
		case MIXERCONTROL_CONTROLTYPE_BOOLEAN:
		case MIXERCONTROL_CONTROLTYPE_MUTE:
			pMixerControlDetailsBoolean[0].fValue = value;
			break;
		default:
			_tprintf(_T("ERROR : Unsupported control type\n"));
			goto cleanUp;
	}

	mr = mixerSetControlDetails((HMIXEROBJ)hMixer, &sMixerControlDetails, MIXER_OBJECTF_HMIXER|MIXER_GETCONTROLDETAILSF_VALUE);
	if (mr != MMSYSERR_NOERROR)
	{
		_tprintf(_T("ERROR : Could not set control details\n"));
		goto cleanUp;
	}

	bRet = TRUE;

cleanUp:

	if (pMixerControls != NULL)
	{
		LocalFree(pMixerControls);
	}

	if (pMixerControlDetailsBoolean != NULL)
	{
		LocalFree(pMixerControlDetailsBoolean);
	}

	if (pMixerControlDetailsUnsigned != NULL)
	{
		LocalFree(pMixerControlDetailsUnsigned);
	}

	if (hMixer != NULL)
	{
		mixerClose(hMixer);
	}

	if (hWaveIn != NULL)
	{
		waveInClose(hWaveIn);
	}

    if (hevDone != NULL) 
	{
        CloseHandle(hevDone);
    }

    return bRet;
}

int _tmain(int argc, TCHAR *argv[], TCHAR *envp[])
{
	UNREFERENCED_PARAMETER(envp);

	if (argc < 2)
	{
		Usage();
		return 0;
	}

	if (0 == wcscmp(argv[1], TEXT("-l")))
	{
		if (argc == 3)
		{
			if (0 == wcscmp(argv[2], TEXT("IN")))
			{
				ListMixerControls(TRUE);
			}
			else if (0 == wcscmp(argv[2], TEXT("OUT")))
			{
				ListMixerControls(FALSE);
			}

			goto exit;
		}

		ListMixerControls(TRUE);
		ListMixerControls(FALSE);
	}
	else if (0 == wcscmp(argv[1], TEXT("-s")))
	{
		if (argc == 5)
		{
			BOOL bIn = FALSE;

			if (0 == wcscmp(argv[2], TEXT("IN")))
			{
				bIn = TRUE;
			}
			else if (0 == wcscmp(argv[2], TEXT("OUT")))
			{
				bIn = FALSE;
			}
			else
			{
				_tprintf(_T("ERROR : You must specify IN or OUT with -s parameter\n"));
				_tprintf(_T("\ti.e. wavemixer -s IN <Control ID> <Value>\n"));
				goto exit;
			}
			
			SetControlValue(bIn, _wtoi(argv[3]), _wtoi(argv[4]));

			goto exit;
		}

		_tprintf(_T("ERROR : Not enough arguments passed with -s parameter\n"));
		_tprintf(_T("\ti.e. wavemixer -s IN 1 0\n"));
	}
	else if (0 == wcscmp(argv[1], TEXT("-?")))
	{
		Usage();
	}

exit:

	return 0;
}

