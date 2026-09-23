/*
 * SPDX-FileCopyrightText: 1998-2002 Pham Kim Long <unikey@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "charset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vnconv.h"

DllExport int genConvert(VnCharset &incs, VnCharset &outcs, ByteInStream &input,
                         ByteOutStream &output) {
    StdVnChar stdChar;
    int bytesRead, bytesWritten;

    incs.startInput();
    outcs.startOutput();

    int ret = 1;
    while (!input.eos()) {
        stdChar = 0;
        if (incs.nextInput(input, stdChar, bytesRead)) {
            if (stdChar != INVALID_STD_CHAR) {
                if (VnCharsetLibObj.m_options.toLower)
                    stdChar = StdVnToLower(stdChar);
                else if (VnCharsetLibObj.m_options.toUpper)
                    stdChar = StdVnToUpper(stdChar);
                if (VnCharsetLibObj.m_options.removeTone)
                    stdChar = StdVnGetRoot(stdChar);
                ret = outcs.putChar(output, stdChar, bytesWritten);
            }
        } else
            break;
    }
    return (ret ? 0 : VNCONV_OUT_OF_MEMORY);
}

//----------------------------------------------
// Arguments:
//       inCharset: charset of input
//       outCharset: charset of output
//       input: input data
//       output: output data
//       inLen: [in]  size of input. if inLen = -1, input data is
//       null-terminated.
//              [out] if input inLen != -1, output iLen is the numbers of byte
//              left in input.
//       maxOutLen: [in] size of output.
//                  [out] number of bytes output, if enough memory
//                        number of bytes needed for output, if not enough
//                        memory
// Returns:  0 if successful
//           error code: if failed
//----------------------------------------------
// int VnConvert(int inCharset, int outCharset, UKBYTE *input, UKBYTE *output,
// int & inLen, int & maxOutLen)

DllExport int VnConvert(int inCharset, int outCharset, UKBYTE *input,
                        UKBYTE *output, int *pInLen, int *pMaxOutLen) {
    int inLen, maxOutLen;
    int ret = -1;

    inLen = *pInLen;
    maxOutLen = *pMaxOutLen;

    if (inLen != -1 && inLen < 0) // invalid inLen
        return ret;

    VnCharset *pInCharset = VnCharsetLibObj.getVnCharset(inCharset);
    VnCharset *pOutCharset = VnCharsetLibObj.getVnCharset(outCharset);

    if (!pInCharset || !pOutCharset)
        return VNCONV_INVALID_CHARSET;

    StringBIStream is(input, inLen, pInCharset->elementSize());
    StringBOStream os(output, maxOutLen);

    ret = genConvert(*pInCharset, *pOutCharset, is, os);
    *pMaxOutLen = os.getOutBytes();
    *pInLen = is.left();
    return ret;
}

const char *ErrTable[VNCONV_LAST_ERROR] = {
    "No error",
    "Unknown error",
    "Invalid charset",
    "Error opening input file",
    "Error opening output file",
    "Error writing to output stream",
    "Not enough memory",
};

DllExport const char *VnConvErrMsg(int errCode) {
    if (errCode < 0 || errCode >= VNCONV_LAST_ERROR)
        errCode = VNCONV_UNKNOWN_ERROR;
    return ErrTable[errCode];
}
