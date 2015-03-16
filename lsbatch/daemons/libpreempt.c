/*
 * Copyright (C) 2015 David Bigagli
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA
 *
 */
#include "preempt.h"

/* prm_init()
 */
int
prm_init(LIST_T *qList)
{
    return 0;
}

/* Elect jobs to be preempted
 */
int
prm_elect_preempt(struct qData *qPtr, link_t *rl, uint32_t numjobs)
{
    link_t *jl;
    struct jData *jPtr;
    struct jData *jPtr2;
    uint32_t numPEND;
    uint32_t numSLOTS;
    linkiter_t iter;

    /* Job in the preemptable candidate list
     * that will other jobs to be requeued.
     */
    jl = make_link();

    /* Gut nicht jobs
     */
    jPtr = qPtr->lastJob;
    if (jPtr == NULL) {
        fin_link(jl);
        return 0;
    }

    numPEND = 0;
    while (jPtr) {

        jPtr2 = jPtr->forw;
        assert(jPtr->jStatus & JOB_STAT_PEND
               || jPtr->jStatus & JOB_STAT_PSUSP);

        if (jPtr->jStatus & JOB_STAT_PEND
            && jPtr->newReason == 0) {
            ++numPEND;
            /* Save the candidate pn jl
             */
            push_link(jl, jPtr);
        }

        /* Fine della coda
         */
        if (jPtr2 == (void *)jDataList[PJL]
            || jPtr->qPtr->priority != jPtr2->qPtr->priority)
            break;
        jPtr = jPtr2;
    }

    if (numPEND == 0) {
        fin_link(jl);
        return 0;
    }

    if (numjobs == 0)
        numjobs = UINT32_MAX;

    /* Traverse candidate list
     */
    while ((jPtr = dequeue_link(jl))) {
        struct qData *qPtr2;
        int num;

        /* Initialiaze the iterator on the list
         * of preemptable queue, the list is
         * traverse in the order in which it
         * was configured.
         */
        traverse_init(jPtr->qPtr->preemptable, &iter);
        numSLOTS = jPtr->shared->jobBill.numProcessors;
        num = 0;

        while ((qPtr2 = traverse_link(&iter))) {
            /* Search on SJL jobs belonging to the
             * preemptable queue and harvest slots.
             * later we want to eventually break out
             * of this loop somehow.
             */
            for (jPtr2 = jDataList[SJL]->forw;
                 jPtr2 != jDataList[SJL];
                 jPtr2 = jPtr2->forw) {

                if (jPtr2->qPtr != qPtr2)
                    continue;
                if (jPtr2->hPtr[0]->hStatus != HOST_STAT_FULL)
                    continue;

                num = num + jPtr2->shared->jobBill.numProcessors;
                push_link(rl, jPtr2);
                if (num >= numSLOTS)
                    goto pryc;
            }
        }
    pryc:;
        if (LINK_NUM_ENTRIES(rl) >= numjobs)
            break;
    }

    fin_link(jl);

    return LINK_NUM_ENTRIES(rl);
}
