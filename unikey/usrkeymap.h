/*
 * SPDX-FileCopyrightText: 2000-2005 Pham Kim Long <unikey@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef __UNIKEY_USER_KEY_MAP_H
#define __UNIKEY_USER_KEY_MAP_H

#include "inputproc.h"
struct UkKeyMapPair {
    unsigned char key;
    int action;
};

DllInterface int UkLoadKeyMap(const char *fileName, int keyMap[256]);
DllInterface int UkLoadKeyOrderMap(const char *fileName, UkKeyMapPair *pMap,
                                   int *pMapCount);
DllInterface int UkStoreKeyOrderMap(const char *fileName, UkKeyMapPair *pMap,
                                    int mapCount);

#endif
