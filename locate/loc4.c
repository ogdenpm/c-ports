/****************************************************************************
 *  loc4.c: part of the C port of Intel's ISIS-II locate                    *
 *  The original ISIS-II application is Copyright Intel                     *
 *                                                                          *
 *  Re-engineered to C by Mark Ogden <mark.pm.ogden@btinternet.com>         *
 *                                                                          *
 *  It is released for academic interest and personal use only              *
 ****************************************************************************/
#include "loc.h"

char const *segNames[256] = { "ABSOLUTE", "CODE", "DATA", "STACK", "MEMORY", "Reserved" };

void PrintMemoryMap(void) {
    if (seen.map) { /* print map header info if asked for */
        PrintListingHeader("MEMORY MAP OF MODULE");
        if (isMain)
            Printf("MODULE START ADDRESS %04XH", startAddr);
        else {
            Printf("MODULE IS NOT A MAIN MODULE");
            if (seen.start)
                Printf(", START CONTROL IGNORED");
            if (seen.restart0)
                Printf(", RESTART CONTROL IGNORED");
        }
        Printf("\n\nSTART   STOP LENGTH REL NAME\n\n");
    }

    segBases[SSTACK] -= segSizes[SSTACK]; //  remove stack seg bias
    uint8_t seg;
    for (int i = 1; i <= 254; i++) {
        // original had test with unused flag 20H, replaced here
        // with FSEGSEEN as it seems appropriate
        if (segSizes[seg = segOrder[i]] || (segFlags[seg] & FSEGSEEN))
            AddSegFrag(segFlags[seg], seg, segBases[seg], segSizes[seg]);
    }

    uint32_t minStart = 0;

    /* emit information on all of the blocks */
    for (int i = 0; i < segFragCnt; i++) {
        uint16_t start = segFrags[i].start;
        uint16_t len   = segFrags[i].len;
        uint8_t flags  = segFrags[i].flags;
        uint8_t seg    = segFrags[i].seg;

        bool addCRLF   = false;
        if (seen.map) { /* if MAP is being produced */
            Printf("%04XH  ", start);
            if (len > 0)
                Printf("%04XH", start + len - 1);
            else
                Printf("****H");
            Printf("  %4XH  %c  ", len, "AIPB"[flags & AMASK]);
            if (seg >= SNAMED) /* seg is COMMON */
                Printf("/%s/", segNames[seg]);
            else /* standard so used the canned names */
                Printf("%-8s", segNames[seg]);
        }
        if (start < minStart) { /* emit any overlap notification */
            if (!(warningMask & 2))
                warnings++;
            addCRLF = true;
            if (len) { // overlap has size
                uint32_t endLoc = start + len - 1;
                if (endLoc >= minStart)
                    endLoc = minStart - 1;
                PrintfAndLog("  (MEMORY OVERLAP FROM %04XH THROUGH %04XH)", start, endLoc);
            } else
                PrintfAndLog("  (MEMORY OVERLAP FROM %04XH THROUGH ****H)", start);
        }
        if (flags & FWRAP0) {
            addCRLF = true;
            PrintfAndLog("  (0 length SEGMENT WRAPPED AROUND TO 0000H)");
        }
        if (addCRLF) {
            PrintfAndLog("\n");
        } else if (seen.map)
            Printf("\n");
        if (len && start + len >= 0x10000) { /* we wrapped to 0 */
            minStart = 0x10000;
        } else if (minStart < 0x10000 &&
                   (uint32_t)(start + len) > minStart) /* update the minStart */
            minStart = start + len;
    }
}