//
//  LPCReader.cpp
//
//  Created by Douglas Scott on 5/1/23.
//

#include "lpcdefs.h"
#include <ugens.h>        // for warn, die
#include <rtcmix_types.h>
#include "utils.h"
#include "LPCDataSet.h"
#include <stdlib.h>
#include <string.h>

extern "C" {
    Handle getlpcpitches(const Arg args[], const int nargs);
}

// getlpcpitches(filename, npoles_guess, first_frame, last_frame [, err_threshold])
// Open an LPC datafile and load the pitch frame values into a list object.
// Any frame whose error value equals or exceeds err_threshold will have its pitch set to zero.
// This allows you to get a list of "valid" pitches for the given threshold.

Handle getlpcpitches(const Arg args[], const int nargs)
{
    LPCDataSet *dataSet = new LPCDataSet;
    const char *filename = args[0];
    const int npolesGuess = (int) (double) args[1];
    int frms = (int) dataSet->open(filename, npolesGuess, 0);   // note: this will fail if file has no header
    int lastFrame=0, firstFrame=0;
    try {
        if (frms < 0) {
            ::rterror("getlpcpitches", "Failed to open LPC file");
            throw FILE_ERROR;
        }

        dataSet->ref();
        int frameCount = (int) dataSet->getFrameCount();

        rtcmix_advise("getlpcpitches", "Data has %d total frames.", frameCount);

        firstFrame = (int) (double) args[2];
        lastFrame = (int) (double) args[3];
        if (firstFrame >= frameCount) {
            ::rterror("getlpcpitches", "Start frame is beyond end of file");
            throw PARAM_ERROR;
        }
        if (lastFrame >= frameCount) {
            rtcmix_warn("getlpcpitches", "End frame is beyond end of file - truncating");
            lastFrame = frameCount - 1;
        }
        if (firstFrame >= lastFrame) {
            ::rterror("getlpcpitches", "End frame must be > start frame");
            throw PARAM_ERROR;
        }
    } catch (RTCmixStatus status) {
        dataSet->unref();
        rtOptionalThrow(status);
        return NULL;
    }

    // If no threshold entered, pass all frame pitch values.

    float errThreshold = (nargs < 5) ? 1.0 : (float)args[4];
    
    int framesToRead = lastFrame - firstFrame + 1;

    Array *outPitches = (Array *)malloc(sizeof(Array));
    outPitches->len = framesToRead;
    outPitches->data = (double *)malloc(framesToRead * sizeof(double));
    memset(outPitches->data, 0, framesToRead * sizeof(double));
    float coeffs[MAXPOLES+4];
    for (int i = firstFrame; i <= lastFrame; ++i) {
        if (dataSet->getFrame((double) i, coeffs) < 0)
            break;
        outPitches->data[i-firstFrame] = (coeffs[THRESH] < errThreshold) ? coeffs[PITCH] : 0.0;
    }
    rtcmix_advise("getlpcpitches", "Returning list of %d pitches", framesToRead);
    
    // Wrap Array in Handle, and return.  This will return a 'list' to MinC.
    return createArrayHandle(outPitches);
}