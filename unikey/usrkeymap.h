/*
 * SPDX-FileCopyrightText: 2000-2005 Pham Kim Long <unikey@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef __UNIKEY_USER_KEY_MAP_H
#define __UNIKEY_USER_KEY_MAP_H

#include <cstdio>
#include "inputproc.h"
struct UkKeyMapPair {
    unsigned char key;
    int action;
};

DllInterface void UkLoadKeyMap(FILE *f, int keyMap[256]);
DllInterface void UkLoadKeyOrderMap(FILE *f, UkKeyMapPair *pMap,
                                    int *pMapCount);
DllInterface void UkStoreKeyOrderMap(FILE *f, UkKeyMapPair *pMap, int mapCount);

#endif
