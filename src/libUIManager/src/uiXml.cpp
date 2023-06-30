/*-----------------------------------------------------------------------------
Software License for The Fraunhofer FDK MPEG-H Software

Copyright (c) 2018 - 2023 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. and Contributors
All rights reserved.

1. INTRODUCTION

The "Fraunhofer FDK MPEG-H Software" is software that implements the ISO/MPEG
MPEG-H 3D Audio standard for digital audio or related system features. Patent
licenses for necessary patent claims for the Fraunhofer FDK MPEG-H Software
(including those of Fraunhofer), for the use in commercial products and
services, may be obtained from the respective patent owners individually and/or
from Via LA (www.via-la.com).

Fraunhofer supports the development of MPEG-H products and services by offering
additional software, documentation, and technical advice. In addition, it
operates the MPEG-H Trademark Program to ease interoperability testing of end-
products. Please visit www.mpegh.com for more information.

2. COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

* You must retain the complete text of this software license in redistributions
of the Fraunhofer FDK MPEG-H Software or your modifications thereto in source
code form.

* You must retain the complete text of this software license in the
documentation and/or other materials provided with redistributions of
the Fraunhofer FDK MPEG-H Software or your modifications thereto in binary form.
You must make available free of charge copies of the complete source code of
the Fraunhofer FDK MPEG-H Software and your modifications thereto to recipients
of copies in binary form.

* The name of Fraunhofer may not be used to endorse or promote products derived
from the Fraunhofer FDK MPEG-H Software without prior written permission.

* You may not charge copyright license fees for anyone to use, copy or
distribute the Fraunhofer FDK MPEG-H Software or your modifications thereto.

* Your modified versions of the Fraunhofer FDK MPEG-H Software must carry
prominent notices stating that you changed the software and the date of any
change. For modified versions of the Fraunhofer FDK MPEG-H Software, the term
"Fraunhofer FDK MPEG-H Software" must be replaced by the term "Third-Party
Modified Version of the Fraunhofer FDK MPEG-H Software".

3. No PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software. You may use this Fraunhofer FDK MPEG-H Software or modifications
thereto only for purposes that are authorized by appropriate patent licenses.

4. DISCLAIMER

This Fraunhofer FDK MPEG-H Software is provided by Fraunhofer on behalf of the
copyright holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED
WARRANTIES, including but not limited to the implied warranties of
merchantability and fitness for a particular purpose. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE for any direct, indirect,
incidental, special, exemplary, or consequential damages, including but not
limited to procurement of substitute goods or services; loss of use, data, or
profits, or business interruption, however caused and on any theory of
liability, whether in contract, strict liability, or tort (including
negligence), arising in any way out of the use of this software, even if
advised of the possibility of such damage.

5. CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Division Audio and Media Technologies - MPEG-H FDK
Am Wolfsmantel 33
91058 Erlangen, Germany
www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
-----------------------------------------------------------------------------*/

/**************** MPEG-H 3DA User Interaction manager library ******************

   Author(s):   Matthias Neusinger

   Description: MPEG-H UI Manager XML formatting/parsing

*******************************************************************************/

#include "uiXml.h"

#include "uiManagerInternal.h"

#define SMART_XML_OUTPUT

#ifdef SMART_XML_OUTPUT

/* sort */
static void sort(USHORT* data, UINT n) {
  UINT i;

  while (n > 1) {
    for (i = 1; i < n; i++) {
      if (data[i - 1] > data[i]) {
        USHORT tmp = data[i];
        data[i] = data[i - 1];
        data[i - 1] = tmp;
      }
    }
    n--;
  }
}

/* append ID to string */
static void appendIDtoStr(char* str, UCHAR id) {
  UCHAR tmp = id;

  while (*str) str++;

  if (id >= 100) {
    UCHAR digit = tmp / 100;
    *str++ = digit + '0';
    tmp -= digit * 100;
  }

  if (id >= 10) {
    UCHAR digit = tmp / 10;
    *str++ = digit + '0';
    tmp -= digit * 10;
  }

  *str++ = tmp + '0';

  *str = 0;
}

#endif

/* reset XML writer state */
static void resetXmlWriter(UI_MANAGER_XML_WRITER* pWriter) {
  pWriter->nextPresetIdx = XML_START_INDEX;
  pWriter->nextGroupIdx = 0;
  pWriter->nextSwitchGroupMemberIdx = XML_START_INDEX;
}

/* write character */
static inline void writeChar(UI_MANAGER_XML_WRITER* pWriter, char c) {
  if (!pWriter->nLeft) return;
  *(pWriter->pOut++) = c;
  pWriter->nLeft--;
}

/* write string */
static void writeString(UI_MANAGER_XML_WRITER* pWriter, const char* str) {
  unsigned int l;

  if (!pWriter->nLeft) return;

  l = FDKstrlen(str);
  if (l > pWriter->nLeft) {
    pWriter->nLeft = 0;
    return;
  }

  FDKstrncpy(pWriter->pOut, str, pWriter->nLeft);
  pWriter->pOut += l;
  pWriter->nLeft -= l;
}

/* write escaped string */
static void writeEscapedString(UI_MANAGER_XML_WRITER* pWriter, const char* str) {
  unsigned int l, i;
  int l_max_out = ASI_MAX_DESCRIPTION_LEN;
  /*
    Original Character   XML entity replacement   XML numeric replacement
    < (0x3c)             &lt;                     &#60;
    > (0x3e)             &gt;                     &#62;
    " (0x22)             &quot;                   &#34;
    & (0x26)             &amp;                    &#38;
    ' (0x27)             &apos;                   &#39;
  */
  const char c_lt[] = "&#60;";
  const char c_gt[] = "&#62;";
  const char c_quot[] = "&#34;";
  const char c_amp[] = "&#38;";
  const char c_apos[] = "&#39;";

  if (!pWriter->nLeft) return;

  l = FDKstrlen(str);
  for (i = 0; i < l; i++) {
    UINT chars_len;
    const char* chars_pointer;

    switch (str[i]) {
      case 0x3c: /* < */
        chars_pointer = c_lt;
        chars_len = 5;
        break;
      case 0x3e: /* > */
        chars_pointer = c_gt;
        chars_len = 5;
        break;
      case 0x22: /* " */
        chars_pointer = c_quot;
        chars_len = 5;
        break;
      case 0x26: /* & */
        chars_pointer = c_amp;
        chars_len = 5;
        break;
      case 0x27: /* ' */
        chars_pointer = c_apos;
        chars_len = 5;
        break;
      default:
        chars_pointer = &str[i];
        chars_len = 1;
        break;
    }
    l_max_out -= chars_len;
    if (l_max_out < 0) {
      /* Truncate written output to the maximum length of the corresponding unescaped string to
       * avoid memory issues. */
      return;
    }
    if (chars_len > pWriter->nLeft) {
      pWriter->nLeft = 0;
      return;
    }
    FDKstrncpy(pWriter->pOut, chars_pointer, chars_len);
    pWriter->pOut += chars_len;
    pWriter->nLeft -= chars_len;
  }
}

/* write boolean */
static void writeBool(UI_MANAGER_XML_WRITER* pWriter, int val) {
  if (val) {
    writeString(pWriter, "\"true\"");
  } else {
    writeString(pWriter, "\"false\"");
  }
}

/* write ID */
static void writeID(UI_MANAGER_XML_WRITER* pWriter, UCHAR id) {
  if (!pWriter->nLeft) return;

  writeChar(pWriter, '"');

  UCHAR tmp = id;

  if (id >= 100) {
    UCHAR digit = tmp / 100;
    writeChar(pWriter, digit + '0');
    tmp -= digit * 100;
  }

  if (id >= 10) {
    UCHAR digit = tmp / 10;
    writeChar(pWriter, digit + '0');
    tmp -= digit * 10;
  }

  writeChar(pWriter, tmp + '0');

  writeChar(pWriter, '"');
}

/* write ID */
static void writeUUID(UI_MANAGER_XML_WRITER* pWriter, UCHAR* uuid) {
  int i;

  if (!pWriter->nLeft) return;

  writeChar(pWriter, '"');

  for (i = 0; i < 16; i++) {
    char c;

    if ((i == 4) || (i == 6) || (i == 8) || (i == 10)) {
      writeChar(pWriter, '-');
    }

    c = uuid[i] >> 4;
    if (c < 10)
      c += '0';
    else
      c += 'A' - 10;
    writeChar(pWriter, c);

    c = uuid[i] & 0xF;
    if (c < 10)
      c += '0';
    else
      c += 'A' - 10;
    writeChar(pWriter, c);
  }

  writeChar(pWriter, '"');
}

/* write gain */
static void writeValue(UI_MANAGER_XML_WRITER* pWriter, SHORT val_x2) {
  int i, tmp;

  if (!pWriter->nLeft) return;

  writeChar(pWriter, '"');

  val_x2 = fMax((SHORT)-1999, fMin((SHORT)1999, val_x2));

  if (val_x2 < 0) writeChar(pWriter, '-');

  i = tmp = fAbs(val_x2) >> 1;

  if (i >= 100) {
    UCHAR digit = tmp / 100;
    writeChar(pWriter, digit + '0');
    tmp -= digit * 100;
  }

  if (i >= 10) {
    UCHAR digit = tmp / 10;
    writeChar(pWriter, digit + '0');
    tmp -= digit * 10;
  }

  writeChar(pWriter, tmp + '0');

  writeChar(pWriter, '.');
  writeChar(pWriter, (val_x2 & 1) ? '5' : '0');

  writeChar(pWriter, '"');
}

/* write description */
static void writeDescription(UI_MANAGER_XML_WRITER* pWriter, const ASI_DESCRIPTION* pDescr,
                             const ASI_CONTENT_DATA* pContent, const UCHAR* pPresetKind) {
  if (pContent && pContent->contentKind) {
    writeString(pWriter, "<kind table=\"ContentKindTable\" code=");
    writeID(pWriter, pContent->contentKind);
    writeString(pWriter, "/>\n");
  }

  if (pPresetKind && *pPresetKind) {
    writeString(pWriter, "<kind table=\"PresetTable\" code=");
    writeID(pWriter, *pPresetKind);
    writeString(pWriter, "/>\n");
  }

  if (!pDescr->present) return;

  writeString(pWriter, "<customKind");

  if (pContent && pContent->contentLanguage[0]) {
    writeString(pWriter, " langCode=\"");
    writeChar(pWriter, pContent->contentLanguage[0]);
    writeChar(pWriter, pContent->contentLanguage[1]);
    writeChar(pWriter, pContent->contentLanguage[2]);
    writeChar(pWriter, '"');
  }

  writeString(pWriter, ">\n");

  writeString(pWriter, "<description");
  if (pDescr->language[0]) {
    writeString(pWriter, " langCode=\"");
    writeChar(pWriter, pDescr->language[0]);
    writeChar(pWriter, pDescr->language[1]);
    writeChar(pWriter, pDescr->language[2]);
    writeChar(pWriter, '"');
  }
  writeString(pWriter, ">");

  writeEscapedString(pWriter, pDescr->data);

  writeString(pWriter, "</description>\n");

  writeString(pWriter, "</customKind>\n");
}

/* write interaction params */
static void writeInteractParams(UI_MANAGER_XML_WRITER* pWriter, const ASI_GROUP* pGroup,
                                const UI_STATE_GROUP* pGroupState) {
#ifdef SMART_XML_OUTPUT
  if (pGroupState->allowGainInteractivity) {
#else
  if (pGroup->allowGainInteractivity) {
#endif
    writeString(pWriter, "<prominenceLevelProp min=");
    writeValue(pWriter, (pGroup->interactivityMinGain - 63) << 1);

    writeString(pWriter, " max=");
    writeValue(pWriter, pGroup->interactivityMaxGain << 1);

    writeString(pWriter, " def=");
    writeValue(pWriter, pGroupState->defaultGain);

    writeString(pWriter, " val=");
    writeValue(pWriter, pGroupState->gain);

    writeString(pWriter, " isActionAllowed=");
    writeBool(pWriter, pGroupState->allowGainInteractivity);

    writeString(pWriter, "/>\n");
  }

#ifdef SMART_XML_OUTPUT
  if (pGroupState->allowOnOff) {
#else
  if (pGroup->allowOnOff) {
#endif
    writeString(pWriter, "<mutingProp def=");
    writeBool(pWriter, !pGroup->defaultOnOff);

    writeString(pWriter, " val=");
    writeBool(pWriter, !pGroupState->onOff);

    writeString(pWriter, " isActionAllowed=");
    writeBool(pWriter, pGroupState->allowOnOff);

    writeString(pWriter, "/>\n");
  }

#ifdef SMART_XML_OUTPUT
  if (pGroupState->allowPositionInteractivity) {
#else
  if (pGroup->allowPositionInteractivity) {
#endif
    if ((INT)pGroup->interactivityMaxAzOffset > -(INT)pGroup->interactivityMinAzOffset) {
      writeString(pWriter, "<azimuthProp min=");
      writeValue(pWriter, -3 * pGroup->interactivityMinAzOffset);

      writeString(pWriter, " max=");
      writeValue(pWriter, 3 * pGroup->interactivityMaxAzOffset);

      writeString(pWriter, " def=");
      writeValue(pWriter, 3 * (pGroupState->defaultAzOffset - 128));

      writeString(pWriter, " val=");
      writeValue(pWriter, 3 * (pGroupState->azOffset - 128));

      writeString(pWriter, " isActionAllowed=");
      writeBool(pWriter, pGroupState->allowPositionInteractivity);

      writeString(pWriter, "/>\n");
    }

    if ((INT)pGroup->interactivityMaxElOffset > -(INT)pGroup->interactivityMinElOffset) {
      writeString(pWriter, "<elevationProp min=");
      writeValue(pWriter, -6 * pGroup->interactivityMinElOffset);

      writeString(pWriter, " max=");
      writeValue(pWriter, 6 * pGroup->interactivityMaxElOffset);

      writeString(pWriter, " def=");
      writeValue(pWriter, 6 * (pGroupState->defaultElOffset - 32));

      writeString(pWriter, " val=");
      writeValue(pWriter, 6 * (pGroupState->elOffset - 32));

      writeString(pWriter, " isActionAllowed=");
      writeBool(pWriter, pGroupState->allowPositionInteractivity);

      writeString(pWriter, "/>\n");
    }

    /* distance factor currently not supported */
  }
}

/* write preset */
static void writePreset(HANDLE_UI_MANAGER hUiManager, int presetIdx) {
  UI_MANAGER_XML_WRITER* pWriter = &(hUiManager->xmlWriter);
  const ASI_GROUP_PRESET* preset = &(hUiManager->asi.groupPresets[presetIdx]);

  writeString(pWriter, "<preset id=");
  writeID(pWriter, preset->groupPresetID);

  writeString(pWriter, " isActive=");
  writeBool(pWriter, hUiManager->uiState.activePresetIndex == presetIdx);

  writeString(pWriter, " isDefault=");
  writeBool(pWriter, preset->groupPresetID == getMinPresetID(hUiManager));

  writeString(pWriter, " isAvailable=");
  writeBool(pWriter, hUiManager->uiState.groupPresets[presetIdx].isAvailable);

  writeString(pWriter, ">\n");

#ifdef SMART_XML_OUTPUT
  if (preset->description.present) {
#endif
    writeDescription(pWriter, &preset->description, NULL, &preset->kind);
#ifdef SMART_XML_OUTPUT
  } else {
    ASI_DESCRIPTION descr = {1, {'e', 'n', 'g'}, "Preset "};
    appendIDtoStr(descr.data, preset->groupPresetID);

    writeDescription(pWriter, &descr, NULL, &preset->kind);
  }
#endif

  writeString(pWriter, "</preset>\n");
}

/* write group */
static void writeGroup(HANDLE_UI_MANAGER hUiManager, int groupIdx) {
  UI_MANAGER_XML_WRITER* pWriter = &(hUiManager->xmlWriter);
  const ASI_GROUP* pGroup = &(hUiManager->asi.groups[groupIdx]);
  const UI_STATE_GROUP* pGroupState = &(hUiManager->uiState.groups[groupIdx]);

  writeString(pWriter, "<audioElement id=");
  writeID(pWriter, pGroup->groupID);

  writeString(pWriter, " isAvailable=");
  writeBool(pWriter, pGroupState->isAvailable);

  writeString(pWriter, ">\n");

  writeInteractParams(pWriter, pGroup, pGroupState);

  writeDescription(pWriter, &pGroup->description, pGroup->contPresent ? &pGroup->contentData : NULL,
                   NULL);

  writeString(pWriter, "</audioElement>\n");
}

/* write switch group */
static void writeSwitchGroup(HANDLE_UI_MANAGER hUiManager, int switchGroupIdx) {
  UI_MANAGER_XML_WRITER* pWriter = &(hUiManager->xmlWriter);
  const ASI_SWITCH_GROUP* pSwitchGroup = &(hUiManager->asi.switchGroups[switchGroupIdx]);
  const UI_STATE_SWITCH_GROUP* pSwitchGroupState =
      &(hUiManager->uiState.switchGroups[switchGroupIdx]);
  int grpIdx, i;
  const ASI_GROUP* pGroup;
  ASI_GROUP tmpGroup;
  const UI_STATE_GROUP* pGroupState;
  USHORT sortedIDs[ASI_MAX_SWITCH_GROUP_MEMBERS];

  if (pWriter->nextSwitchGroupMemberIdx == XML_START_INDEX) {
    writeString(pWriter, "<audioElementSwitch id=");
    writeID(pWriter, pSwitchGroup->switchGroupID);

    writeString(pWriter, " isAvailable=");
    writeBool(pWriter, pSwitchGroupState->isAvailable);

    writeString(pWriter, " isActionAllowed=");
#ifdef SMART_XML_OUTPUT
    writeBool(pWriter, pSwitchGroupState->allowSwitch && (pSwitchGroup->numMembers > 1));
#else
    writeBool(pWriter, pSwitchGroupState->allowSwitch);
#endif

    writeString(pWriter, ">\n");

    grpIdx = asiGroupID2idx(&hUiManager->asi,
                            pSwitchGroup->memberID[pSwitchGroupState->activeMemberIndex]);

    if (grpIdx < 0) return;

    pGroup = &(hUiManager->asi.groups[grpIdx]);
    pGroupState = &(hUiManager->uiState.groups[grpIdx]);

    tmpGroup = *pGroup;
    tmpGroup.allowOnOff = pSwitchGroup->allowOnOff;
    tmpGroup.defaultOnOff = pSwitchGroup->defaultOnOff;
    writeInteractParams(pWriter, &tmpGroup, pGroupState);

    writeString(pWriter, "<audioElements>\n");

    if (pWriter->nLeft) {
      pWriter->pLastValidPos = pWriter->pOut;
      pWriter->nextSwitchGroupMemberIdx = 0;
    } else {
      return;
    }
  }

  /* sort switch group members by ID */
  for (i = 0; i < pSwitchGroup->numMembers; i++) {
    sortedIDs[i] = (pSwitchGroup->memberID[i] << 8) | i;
  }
#ifdef SMART_XML_OUTPUT
  sort(sortedIDs, pSwitchGroup->numMembers);
#endif

  for (i = pWriter->nextSwitchGroupMemberIdx; i < pSwitchGroup->numMembers; i++) {
#ifdef SMART_XML_OUTPUT
    int isDefault;
#endif

    grpIdx = asiGroupID2idx(&hUiManager->asi, sortedIDs[i] >> 8);

    if (grpIdx < 0) continue;

    pGroup = &(hUiManager->asi.groups[grpIdx]);
    pGroupState = &(hUiManager->uiState.groups[grpIdx]);

#ifdef SMART_XML_OUTPUT
    if (!pSwitchGroupState->allowSwitch &&
        (pSwitchGroupState->activeMemberIndex != (sortedIDs[i] & 0xFF)))
      continue;
#endif

    writeString(pWriter, "<audioElement id=");
    writeID(pWriter, pGroup->groupID);

    writeString(pWriter, " isActive=");
    writeBool(pWriter, pSwitchGroupState->activeMemberIndex == (sortedIDs[i] & 0xFF));

    writeString(pWriter, " isDefault=");
#ifdef SMART_XML_OUTPUT
    isDefault = pSwitchGroupState->allowSwitch
                    ? (pSwitchGroup->defaultGroupID == pGroup->groupID)
                    : (pSwitchGroupState->activeMemberIndex == (sortedIDs[i] & 0xFF));
    writeBool(pWriter, isDefault);
#else
    writeBool(pWriter, pSwitchGroup->defaultGroupID == pGroup->groupID);
#endif

    writeString(pWriter, " isAvailable=");
    writeBool(pWriter, pGroupState->isAvailable);

    writeString(pWriter, ">\n");

#ifdef SMART_XML_OUTPUT
    if (pGroup->description.present) {
#endif
      writeDescription(pWriter, &pGroup->description,
                       pGroup->contPresent ? &pGroup->contentData : NULL, NULL);
#ifdef SMART_XML_OUTPUT
    } else {
      ASI_DESCRIPTION descr = {1, {'e', 'n', 'g'}, ""};
      appendIDtoStr(descr.data, pGroup->groupID);

      writeDescription(pWriter, &descr, pGroup->contPresent ? &pGroup->contentData : NULL, NULL);
    }
#endif

    writeString(pWriter, "</audioElement>\n");

    if (pWriter->nLeft) {
      pWriter->pLastValidPos = pWriter->pOut;
      pWriter->nextSwitchGroupMemberIdx = i + 1;
    } else {
      return;
    }
  }

  writeString(pWriter, "</audioElements>\n");

  writeDescription(pWriter, &pSwitchGroup->description, NULL, NULL);

  writeString(pWriter, "</audioElementSwitch>\n");

  if (pWriter->nLeft) {
    pWriter->pLastValidPos = pWriter->pOut;
    pWriter->nextSwitchGroupMemberIdx = XML_START_INDEX;
  }
}

/* write DRC info */
static void writeDrcInfo(HANDLE_UI_MANAGER hUiManager) {
  UI_MANAGER_XML_WRITER* pWriter = &(hUiManager->xmlWriter);
  ASI_DRC_UI_INFO* pDrcInfo = &(hUiManager->asi.drcUiInfo);
  int i;
  USHORT drcSetEffectAvailable = 0;
  int targetLoudnessValueLower, targetLoudnessValueUpper;

  if (hUiManager->asi.drcUiInfoPresent) {
    if (hUiManager->uiState.targetLoudness == UI_MANAGER_USE_DEFAULT_TARGET_LOUDNESS) {
      /* target loudness is not known here. Use superset of all drcSetEffectAvailable entries. */
      for (i = 0; i < hUiManager->asi.drcUiInfo.numTargetLoudnessConditions; i++) {
        drcSetEffectAvailable |= pDrcInfo->drcSetEffectAvailable[i];
      }
    } else {
      /* choose drcSetEffectAvailable bitmask for given target loudness */
      targetLoudnessValueLower = -63;
      for (i = 0; i < hUiManager->asi.drcUiInfo.numTargetLoudnessConditions; i++) {
        targetLoudnessValueUpper = pDrcInfo->bsTargetLoudnessValueUpper[i] - 63;
        if ((hUiManager->uiState.targetLoudness > targetLoudnessValueLower) &&
            (hUiManager->uiState.targetLoudness <= targetLoudnessValueUpper)) {
          drcSetEffectAvailable = pDrcInfo->drcSetEffectAvailable[i];
          break;
        }
        targetLoudnessValueLower = targetLoudnessValueUpper;
      }
    }
  } else {
    /* if no information is present, all bits shall be set to one */
    drcSetEffectAvailable = 0x7F;
  }

  writeString(pWriter, "<DRCInfo>\n");

  for (i = 0; i < 7; i++) {
    if (drcSetEffectAvailable & (1 << i)) {
      writeString(pWriter, "<drcSetEffectAvailable index=");
      writeID(pWriter, i);
      writeString(pWriter, " />\n");
    }
  }

  writeString(pWriter, "</DRCInfo>\n");
}

/* write scene */
static void writeScene(HANDLE_UI_MANAGER hUiManager, UCHAR shortInfo) {
  UI_MANAGER_XML_WRITER* pWriter = &(hUiManager->xmlWriter);
  USHORT sortedIDs[ASI_MAX_GROUPS + ASI_MAX_SWITCH_GROUPS], mainDlg = 0xFFFF;
  int i, n;

  if (pWriter->nextPresetIdx == XML_START_INDEX) {
    /* start XML */
    writeString(pWriter, "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n");

    /* start AudioSceneConfig */
    if (shortInfo == 2) { /* 2 indicates no UI available */
      writeString(pWriter, "<AudioSceneConfig uuid=\"00000000-0000-0000-0000-000000000000\"");
    } else {
      writeString(pWriter, "<AudioSceneConfig uuid=");
      writeUUID(pWriter, hUiManager->uiState.uuid);
    }
    writeString(pWriter, " version=\"9.0\"");
    writeString(pWriter, " configChanged=");
    if (shortInfo == 2) { /* 2 indicates no UI available */
      writeBool(pWriter, 0);
    } else {
      writeBool(pWriter, hUiManager->configChanged);
    }

    if (shortInfo) {
      writeString(pWriter, "/>\n");
      return;
    }

    writeString(pWriter, ">\n");

    /* write DRC info */
    writeDrcInfo(hUiManager);

    /* start presets */
    writeString(pWriter, "<presets>\n");

    if (pWriter->nLeft) {
      pWriter->pLastValidPos = pWriter->pOut;
      pWriter->nextPresetIdx = 0;
    } else {
      return;
    }
  }

#ifdef SMART_XML_OUTPUT
  if (hUiManager->asi.numGroupPresets == 0) {
    /* if no preset present show default preset */
    if (pWriter->nextPresetIdx == 0) {
      writeString(pWriter,
                  "<preset id=\"0\" isActive=\"true\" isDefault=\"true\" isAvailable=\"true\">\n");
      writeString(pWriter, "<customKind>\n");
      writeString(pWriter, "<description langCode=\"eng\">Default</description>\n");
      writeString(pWriter, "</customKind>\n");
      writeString(pWriter, "</preset>\n");

      if (pWriter->nLeft) {
        pWriter->pLastValidPos = pWriter->pOut;
        pWriter->nextPresetIdx = 1;
      } else {
        return;
      }
    }
  } else
#endif
  {
    /* sort presets by ID */
    for (i = 0; i < hUiManager->asi.numGroupPresets; i++) {
      sortedIDs[i] = (hUiManager->asi.groupPresets[i].groupPresetID << 8) | i;
    }
#ifdef SMART_XML_OUTPUT
    sort(sortedIDs, hUiManager->asi.numGroupPresets);
#endif

    /* write presets */
    for (i = pWriter->nextPresetIdx; i < hUiManager->asi.numGroupPresets; i++) {
      writePreset(hUiManager, sortedIDs[i] & 0xFF);

      if (pWriter->nLeft) {
        pWriter->pLastValidPos = pWriter->pOut;
        pWriter->nextPresetIdx = i + 1;
      } else {
        return;
      }
    }
  }

  /* end presets */
  if (pWriter->nextPresetIdx != XML_END_INDEX) {
    writeString(pWriter, "</presets>\n");
    if (pWriter->nLeft) {
      pWriter->pLastValidPos = pWriter->pOut;
      pWriter->nextPresetIdx = XML_END_INDEX;
    } else {
      return;
    }
  }

  /* sort groups and switch groups by ID, but main dialog element first */
  n = 0;
  for (i = 0; i < hUiManager->asi.numGroups; i++) {
    USHORT tmp = (hUiManager->asi.groups[i].groupID << 9) | i;

    /* find main dialog ID */
    if (hUiManager->asi.groups[i].contPresent &&
        hUiManager->asi.groups[i].contentData.contentKind == 2) {
      if (tmp < mainDlg) mainDlg = tmp;
    }

    /* check if group is switch group member*/
    if (hUiManager->asi.groups[i].switchGroupID != INVALID_ID) continue;

    sortedIDs[n] = tmp;
    n++;
  }

  for (i = 0; i < hUiManager->asi.numSwitchGroups; i++) {
    sortedIDs[n] = (hUiManager->asi.switchGroups[i].switchGroupID << 9) | 0x100 | i;
    n++;
  }

#ifdef SMART_XML_OUTPUT
  sort(sortedIDs, n);

  if (mainDlg != 0xFFFF) {
    UCHAR swGrpID = hUiManager->asi.groups[mainDlg & 0xFF].switchGroupID;

    if (swGrpID != INVALID_ID) {
      int swGrpIdx = asiSwitchGroupID2idx(&hUiManager->asi, swGrpID);
      if (swGrpIdx >= 0) mainDlg = (swGrpID << 9) | 0x100 | swGrpIdx;
    }

    for (i = 1; i < n; i++) {
      if (sortedIDs[i] == mainDlg) {
        FDKmemmove(sortedIDs + 1, sortedIDs, i * sizeof(USHORT));
        sortedIDs[0] = mainDlg;
        break;
      }
    }
  }
#endif

  /* write groups and switch groups */
  for (i = pWriter->nextGroupIdx; i < n; i++) {
    if (sortedIDs[i] & 0x100) {
#ifdef SMART_XML_OUTPUT
      const UI_STATE_SWITCH_GROUP* pSwitchGroupState =
          &(hUiManager->uiState.switchGroups[sortedIDs[i] & 0xFF]);
      int grpIdx =
          asiGroupID2idx(&hUiManager->asi, hUiManager->asi.switchGroups[sortedIDs[i] & 0xFF]
                                               .memberID[pSwitchGroupState->activeMemberIndex]);
      const UI_STATE_GROUP* pGroupState;
      int allowSwitch = pSwitchGroupState->allowSwitch &&
                        (hUiManager->asi.switchGroups[sortedIDs[i] & 0xFF].numMembers > 1);
      int allowAnyAction;

      if (grpIdx < 0) continue;
      pGroupState = &(hUiManager->uiState.groups[grpIdx]);

      /* show switch group only if any interaction allowed */
      allowAnyAction =
          (allowSwitch || pGroupState->allowOnOff || pGroupState->allowGainInteractivity ||
           pGroupState->allowPositionInteractivity);
      if (!pGroupState->onOff && !pGroupState->allowOnOff) allowAnyAction = 0;

      if (allowAnyAction)
#endif
      {
        /* write switch group */
        writeSwitchGroup(hUiManager, sortedIDs[i] & 0xFF);
      }
    } else {
#ifdef SMART_XML_OUTPUT
      const UI_STATE_GROUP* pGroupState = &(hUiManager->uiState.groups[sortedIDs[i] & 0xFF]);
      int allowAnyAction;

      /* show group only if any interaction allowed */
      allowAnyAction = (pGroupState->allowOnOff || pGroupState->allowGainInteractivity ||
                        pGroupState->allowPositionInteractivity);
      if (!pGroupState->onOff && !pGroupState->allowOnOff) allowAnyAction = 0;

      if (allowAnyAction)
#endif
      {
        /* write group */
        writeGroup(hUiManager, sortedIDs[i] & 0xFF);
      }
    }

    if (pWriter->nLeft) {
      pWriter->pLastValidPos = pWriter->pOut;
      pWriter->nextGroupIdx = i + 1;
    } else {
      return;
    }
  }

  /* end AudioSceneConfig */
  writeString(pWriter, "</AudioSceneConfig>\n");

  if (pWriter->nLeft) {
    resetXmlWriter(pWriter);
  }
}

/* write complete XML string */
UI_MANAGER_ERROR uiManagerWriteXML(HANDLE_UI_MANAGER hUiManager, char* xmlOut, UINT xmlOutSize,
                                   UINT flagsIn, UINT* flagsOut) {
  hUiManager->xmlWriter.pOut = xmlOut;
  hUiManager->xmlWriter.nLeft = xmlOutSize;
  hUiManager->xmlWriter.pLastValidPos = NULL;

  if (flagsIn & UI_MANAGER_FORCE_RESTART_XML) {
    resetXmlWriter(&hUiManager->xmlWriter);
  }

  if (hUiManager->xmlWriter.nextPresetIdx != XML_START_INDEX) {
    *flagsOut |= UI_MANAGER_CONTINUES_XML;
  }

  if (hUiManager->configChanged) {
    *flagsOut |= UI_MANAGER_SHORT_OUTPUT;
  }

  {
    UCHAR shortInfo = (*flagsOut & UI_MANAGER_SHORT_OUTPUT) != 0;

    if (hUiManager->isActive == 0) {
      shortInfo = 2; /* 2 indicates no UI available */
    }
    writeScene(hUiManager, shortInfo);
  }

  if (!hUiManager->xmlWriter.nLeft) {
    if (!hUiManager->xmlWriter.pLastValidPos) {
      /* buffer too small to write anything */
      xmlOut[0] = 0;
      return UI_MANAGER_BUFFER_TOO_SMALL;
    }

    /* terminate string after last complete element */
    hUiManager->xmlWriter.pLastValidPos[0] = 0;
    *flagsOut |= UI_MANAGER_INCOMPLETE_XML;
  }

  /* terminate string */
  writeChar(&hUiManager->xmlWriter, 0);

  return UI_MANAGER_OK;
}

/* find char in string */
static UINT findChar(char charToFind, const char* strToSearchIn, UINT lenToSearchIn) {
  UINT i;

  for (i = 0; i < lenToSearchIn; i++) {
    if (strToSearchIn[i] == charToFind) return i + 1;
  }

  return 0;
}

/* find string */
static UINT findString(const char* strToFind, const char* strToSearchIn, UINT lenToSearchIn) {
  UINT i, j, l;

  l = FDKstrlen(strToFind);

  if (lenToSearchIn < l) return 0;

  for (i = 0; i <= lenToSearchIn - l; i++) {
    for (j = 0; j < l; j++) {
      if (strToSearchIn[i + j] != strToFind[j]) break;
    }
    if (j == l) return i + j;
  }

  return 0;
}

/* find XML attribute */
static const char* findAttrib(const char* attrib, const char* strToSearchIn, UINT lenToSearchIn) {
  const char* p = strToSearchIn;
  int i, l = lenToSearchIn;

  /* find attribute name */
  i = findString(attrib, p, l);
  if (!i) return NULL;
  p += i;
  l -= i;

  /* find = */
  i = findChar('=', p, l);
  if (!i) return NULL;
  p += i;
  l -= i;

  /* find opening " */
  i = findChar('"', p, l);
  if (!i) return NULL;
  p += i;
  l -= i;

  /* find closing " */
  i = findChar('"', p, l);
  if (!i) return NULL;

  return p;
}

/* read int */
static INT readInt(const char* str) {
  INT i = 0;
  int sgn = 1;

  /* sign */
  if (*str == '-') {
    sgn = -1;
    str++;
  }

  while (*str != '"') {
    if ((*str < '0') || (*str > '9')) return 0;
    i = 10 * i + *str - '0';
    str++;
  }

  if (sgn < 0) i = -i;

  return i;
}

/* read float */
static FIXP_DBL readFloat(const char* str) {
  /* parse float value */
  LONG i = 0, f = 0, nf = 1;
  int sgn = 0;
  FIXP_DBL res;

  /* sign */
  if (*str == '-') {
    sgn = -1;
    str++;
  } else if (*str == '+') {
    sgn = 1;
    str++;
  }

  /* integer part */
  while ((*str != '"') && (*str != '.')) {
    if ((*str < '0') || (*str > '9')) return 0;
    i = 10 * i + *str - '0';
    str++;
  }

  /* fractional part */
  if (*str == '.') str++;
  while (*str != '"') {
    if ((*str < '0') || (*str > '9')) return 0;
    f = 10 * f + *str - '0';
    nf *= 10;
    if (nf >= 10000) break;
    str++;
  }

  /* store result in Q16 format */
  res = (i << 16) + ((f << 16) / nf);
  if (sgn < 0) res = -res;

  return res;
}

/* read text */
static void readText(const char* str, char* text) {
  int i;

  for (i = 0; i < MAX_XML_TEXT_LENGTH; i++) {
    if (str[i] == '"') break;

    text[i] = str[i];
  }

  text[i] = 0;
}

/* read boolean */
static UCHAR readBool(const char* str) {
  if (FDKstrncmp(str, "true", 4) == 0) return 1;
  if (FDKstrncmp(str, "false", 4) == 0) return 0;
  return readInt(str) ? 1 : 0;
}

/* read UUID */
static void readUUID(const char* str, UCHAR* uuid) {
  int i;

  for (i = 0; i < 16; i++) {
    UCHAR tmp;

    if ((i == 4) || (i == 6) || (i == 8) || (i == 10)) {
      if (*str != '-') break;
      str++;
    }

    if ((*str >= '0') && (*str <= '9'))
      tmp = *str - '0';
    else if ((*str >= 'A') && (*str <= 'F'))
      tmp = *str - 'A' + 10;
    else if ((*str >= 'a') && (*str <= 'f'))
      tmp = *str - 'a' + 10;
    else
      break;

    str++;
    tmp <<= 4;

    if ((*str >= '0') && (*str <= '9'))
      tmp += *str - '0';
    else if ((*str >= 'A') && (*str <= 'F'))
      tmp += *str - 'A' + 10;
    else if ((*str >= 'a') && (*str <= 'f'))
      tmp += *str - 'a' + 10;
    else
      break;

    str++;
    uuid[i] = tmp;
  }
}

/* parse XML action */
UINT uiManagerParseXmlAction(const char* xmlIn, UINT xmlInSize, UI_MANAGER_ACTION* pAction) {
  int i, l = xmlInSize;
  const char *p = xmlIn, *pAttrib;

  FDKmemclear(&pAction->uuid, 16 * sizeof(UCHAR));
  pAction->actionType = UI_MANAGER_COMMAND_INVALID;
  pAction->paramInt = 0;
  pAction->paramFloat = (FIXP_DBL)0;
  pAction->paramBool = 0;
  pAction->paramText[0] = 0;
  pAction->presentFlags = 0;

  /* find ActionEvent tag */
  i = findString("<ActionEvent", p, l);
  if (!i) return 0;
  p += i;
  l -= i;

  /* find end of ActionEvent */
  i = findString("/>", p, l);
  if (!i) return 0;
  l = i - 2;

  /* parse uuid attribute */
  pAttrib = findAttrib("uuid", p, l);
  if (!pAttrib) return 0;
  readUUID(pAttrib, pAction->uuid);

  /* parse version attribute */
  pAttrib = findAttrib("version", p, l);
  if (!pAttrib) return 0;
  if (readFloat(pAttrib) != (FIXP_DBL)0x90000) return 0;

  /* parse actionType attribute */
  pAttrib = findAttrib("actionType", p, l);
  if (!pAttrib) return 0;
  pAction->actionType = readInt(pAttrib);

  /* parse paramInt attribute */
  pAttrib = findAttrib("paramInt", p, l);
  if (pAttrib) {
    pAction->paramInt = readInt(pAttrib);
    pAction->presentFlags |= FLAG_XML_PARAM_INT;
  }

  /* parse paramFloat attribute */
  pAttrib = findAttrib("paramFloat", p, l);
  if (pAttrib) {
    pAction->paramFloat = readFloat(pAttrib);
    pAction->presentFlags |= FLAG_XML_PARAM_FLOAT;
  }

  /* parse paramBool attribute */
  pAttrib = findAttrib("paramBool", p, l);
  if (pAttrib) {
    pAction->paramBool = readBool(pAttrib);
    pAction->presentFlags |= FLAG_XML_PARAM_BOOL;
  }

  /* parse paramText attribute */
  pAttrib = findAttrib("paramText", p, l);
  if (pAttrib) {
    readText(pAttrib, pAction->paramText);
    pAction->presentFlags |= FLAG_XML_PARAM_TEXT;
  }

  /* Note: (p - xmlIn) is always positive */
  return (UINT)(p - xmlIn) + l + 2;
}
