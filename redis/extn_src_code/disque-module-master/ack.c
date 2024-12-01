/* Acknowledges handling and commands.
 *
 * Copyright (c) 2014-2019, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved. This code is under the AGPL license, check the
 * LICENSE file for more info.
 */

#include "disque.h"

/* ------------------------- Low level ack functions ------------------------ */

/* Change job state as acknowledged. If it is already in that state, the
 * function does nothing. */
void acknowledgeJob(RedisModuleCtx *ctx, job *job) {
    if (job->state == JOB_STATE_ACKED) return;

    dequeueJob(job);
    job->state = JOB_STATE_ACKED;
    /* Remove the nodes_confirmed hash table if it exists.
     * tryJobGC() will take care to create a new one used for the GC
     * process. */
    if (job->nodes_confirmed) {
        raxFree(job->nodes_confirmed);
        job->nodes_confirmed = NULL;
    }
    updateJobAwakeTime(job,0); /* Make sure we'll schedule a job GC. */
    AOFAckJob(ctx,job); /* Change job state in AOF. */
}

/* ------------------------- Garbage collection ----------------------------- */

/* Return the next milliseconds unix time where the next GC attempt for this
 * job should be performed. */
mstime_t getNextGCRetryTime(job *job) {
    mstime_t period = JOB_GC_RETRY_MIN_PERIOD * (1 << job->gc_retry);
    if (period > JOB_GC_RETRY_MAX_PERIOD) period = JOB_GC_RETRY_MAX_PERIOD;
    /* Desync a bit the GC process, it is a waste of resources for
     * multiple nodes to try to GC at the same time. */
    return mstime() + period + randomTimeError(500);
}

/* Try to garbage collect the job. */
void tryJobGC(RedisModuleCtx *ctx, job *job) {
    if (job->state != JOB_STATE_ACKED) return;

    int dummy_ack = raxSize(job->nodes_delivered) == 0;
    RedisModule_Log(ctx,"verbose","GC %.*s",JOB_ID_LEN,job->id);

    /* Don't overflow the count, it's only useful for the exponential delay.
     * Actually we'll keep trying forever. */
    if (job->gc_retry != JOB_GC_RETRY_COUNT_MAX) job->gc_retry++;

    /* nodes_confirmed is used in order to store all the nodes that have the
     * job in ACKed state, so that the job can be evicted when we are
     * confident the job will not be reissued. */
    if (job->nodes_confirmed == NULL) {
        job->nodes_confirmed = raxNew();
        const char *myself = RedisModule_GetMyClusterID();
        raxInsert(job->nodes_confirmed,(unsigned char*)myself,REDISMODULE_NODE_ID_LEN,NULL,NULL);
    }

    /* Check ASAP if we already reached all the nodes. This special case
     * here is mainly useful when the job replication factor is 1, so
     * there is no SETACK to send, nor GOTCAK to receive.
     *
     * Also check if this is a dummy ACK but the cluster size is now 1:
     * in such a case we don't have other nodes to send SETACK to, we can
     * just remove the ACK. Note that dummy ACKs are not created at all
     * if the cluster size is 1, but this code path may be entered as a result
     * of the cluster getting resized to a single node. */
    int all_nodes_reached =
        (!dummy_ack) &&
        (raxSize(job->nodes_delivered) == raxSize(job->nodes_confirmed));
    int dummy_ack_single_node = dummy_ack && RedisModule_GetClusterSize() == 1;

    if (all_nodes_reached || dummy_ack_single_node) {
        RedisModule_Log(ctx,"verbose",
            "Deleting %.*s: all nodes reached in tryJobGC()",
            JOB_ID_LEN, job->id);
        unregisterJob(ctx,job);
        freeJob(job);
        return;
    }

    /* Send a SETACK message to all the nodes that may have a message but are
     * still not listed in the nodes_confirmed hash table. However if this
     * is a dummy ACK (created by ACKJOB command acknowledging a job we don't
     * know) we have to broadcast the SETACK to everybody in search of the
     * owner. */
    if (raxSize(job->nodes_delivered)) {
        raxIterator ri;
        raxStart(&ri,job->nodes_delivered);
        raxSeek(&ri,"^",NULL,0);
        while(raxNext(&ri)) {
            RedisModule_Log(ctx,"verbose",
                "GC: sending SETACK to %.*s",
                REDISMODULE_NODE_ID_LEN, ri.key);
            clusterSendSetAck(ctx,(char*)ri.key,job);
        }
        raxStop(&ri);
    } else {
        RedisModule_Log(ctx,"verbose",
            "GC: sending SETACK to the whole cluster");
        clusterSendSetAck(ctx,NULL,job);
    }
}

/* This function is called by cluster.c every time we receive a GOTACK message
 * about a job we know. */
void gotAckReceived(RedisModuleCtx *ctx, const char *sender, job *job, int known) {
    /* A dummy ACK is an acknowledged job that we created just because a client
     * sent us ACKJOB about a job we were not aware. */
    int dummy_ack = raxSize(job->nodes_delivered) == 0;

    RedisModule_Log(ctx,"verbose",
        "RECEIVED GOTACK FROM %.*s FOR JOB %.*s",
        REDISMODULE_NODE_ID_LEN, sender, JOB_ID_LEN, job->id);

    /* We should never receive a GOTACK for a job which is not acknowledged,
     * but it is more robust to handle it explicitly. */
    if (job->state != JOB_STATE_ACKED) return;

    /* If this is a dummy ACK, and we reached a node that knows about this job,
     * it's up to it to perform the garbage collection, so we can forget about
     * this job and reclaim memory. */
    if (dummy_ack && known) {
        RedisModule_Log(ctx,"verbose",
            "Deleting %.*s: authoritative node reached",
            JOB_ID_LEN, job->id);
        unregisterJob(ctx,job);
        freeJob(job);
        return;
    }

    /* If the sender knows about the job, or if we have the sender in the list
     * of nodes that may have the job (even if it no longer remembers about
     * the job), we do two things:
     *
     * 1) Add the node to the list of nodes_delivered. It is likely already
     *    there... so this should be useless, but is a good invariant
     *    to enforce.
     * 2) Add the node to the list of nodes that acknowledged the ACK. */
    if (known ||
        raxFind(job->nodes_delivered,(unsigned char*)sender,
                REDISMODULE_NODE_ID_LEN) != NULL)
    {
        raxInsert(job->nodes_delivered,(unsigned char*)sender,
                  REDISMODULE_NODE_ID_LEN,NULL,NULL);
        /* job->nodes_confirmed exists if we started a job garbage collection,
         * but we may receive GOTACK messages in other conditions sometimes,
         * since we reply with SETACK to QUEUED and WILLQUEUE if the job is
         * acknowledged but we did not yet started to GC. So we need to test
         * if the hash table actually exists. */
        if (job->nodes_confirmed)
            raxInsert(job->nodes_confirmed,(unsigned char*)sender,
                      REDISMODULE_NODE_ID_LEN,NULL,NULL);
    }

    /* If our job is actually a dummy ACK, we are still interested to collect
     * all the nodes in the cluster that reported they don't have a clue:
     * eventually if everybody in the cluster has no clue, we can safely remove
     * the dummy ACK. */
    if (!known && dummy_ack) {
        raxInsert(job->nodes_confirmed,(unsigned char*)sender,
                  REDISMODULE_NODE_ID_LEN,NULL,NULL);
        if (raxSize(job->nodes_confirmed) >= RedisModule_GetClusterSize()) {
            RedisModule_Log(ctx,"verbose",
                "Deleting %.*s: dummy ACK not known cluster-wide",
                JOB_ID_LEN, job->id);
            unregisterJob(ctx,job);
            freeJob(job);
            return;
        }
    }

    /* Finally the common case: our SETACK reached everybody. Broadcast
     * a DELJOB to all the nodes involved, and delete the job. */
    if (!dummy_ack && job->nodes_confirmed &&
         raxSize(job->nodes_confirmed) >= raxSize(job->nodes_delivered))
    {
        RedisModule_Log(ctx,"verbose",
            "Deleting %.*s: All nodes involved acknowledged the job",
            JOB_ID_LEN, job->id);
        clusterBroadcastDelJob(ctx,job);
        unregisterJob(ctx,job);
        freeJob(job);
        return;
    }
}

/* --------------------------  Acks related commands ------------------------ */

/* ACKJOB jobid_1 jobid_2 ... jobid_N
 *
 * Set job state as acknowledged, if the job does not exist creates a
 * fake job just to hold the acknowledge.
 *
 * As a result of a job being acknowledged, the system tries to garbage
 * collect it, that is, to remove the job from every node of the system
 * in order to both avoid multiple deliveries of the same job, and to
 * release resources.
 *
 * If a job was already acknowledged, the ACKJOB command still has the
 * effect of forcing a GC attempt ASAP.
 *
 * The command returns the number of jobs already known and that were
 * already not in the ACKED state.
 */
int ackjobCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    int j, known = 0;
    int cluster_size = RedisModule_GetClusterSize();

    if (validateJobIDs(ctx,argv+1,argc-1) == C_ERR) return REDISMODULE_OK;

    /* Perform the appropriate action for each job. */
    for (j = 1; j < argc; j++) {
        const char *jobid = RedisModule_StringPtrLen(argv[j],NULL);
        job *job = lookupJob(jobid);
        /* Case 1: No such job. Create one just to hold the ACK. However
         * if the cluster is composed by a single node we are sure the job
         * does not exist in the whole cluster, so do this only if the
         * cluster size is greater than one. */
        if (job == NULL && cluster_size > 1 && !myselfLeaving()) {
            const char *id = RedisModule_StringPtrLen(argv[j],NULL);
            int ttl = getRawTTLFromJobID(id);

            /* TTL is even for "at most once" jobs. In this case we
             * don't need to create a dummy hack. */
            if (ttl & 1) {
                job = createJob(id,JOB_STATE_ACKED,0,0);
                setJobTTLFromID(job);
                RedisModule_Assert(registerJob(job) == C_OK);
            }
        }
        /* Case 2: Job exists and is not acknowledged. Change state. */
        else if (job && job->state != JOB_STATE_ACKED) {
            dequeueJob(job); /* Safe to call if job is not queued. */
            acknowledgeJob(ctx,job);
            known++;
        }
        /* Anyway... start a GC attempt on the acked job. */
        if (job) tryJobGC(ctx,job);
    }
    return RedisModule_ReplyWithLongLong(ctx,known);
}

/* FASTACK jobid_1 jobid_2 ... jobid_N
 *
 * Performs a fast acknowledge of the specified jobs.
 * A fast acknowledge does not really attempt to make sure all the nodes
 * that may have a copy receive the ack. The job is just discarded and
 * a best-effort DELJOB is sent to all the nodes that may have a copy
 * without caring if they receive or not the message.
 *
 * This command will more likely result in duplicated messages delivery
 * during network partitions, but uses less messages compared to ACKJOB.
 *
 * If a job is not known, a cluster-wide DELJOB is broadcasted.
 *
 * The command returns the number of jobs that are deleted from the local
 * node as a result of receiving the command.
 */
int fastackCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
    int j, known = 0;

    if (validateJobIDs(ctx,argv+1,argc-1) == C_ERR) return REDISMODULE_OK;

    /* Perform the appropriate action for each job. */
    for (j = 1; j < argc; j++) {
        job *job = lookupJob(RedisModule_StringPtrLen(argv[j],NULL));
        if (job == NULL) {
            /* Job not known, just broadcast the DELJOB message to everybody. */
            clusterBroadcastJobIDMessage(ctx,NULL,
                RedisModule_StringPtrLen(argv[j],NULL),
                DISQUE_MSG_DELJOB,0,
                DISQUE_MSG_NOFLAGS);
        } else {
            clusterBroadcastDelJob(ctx,job);
            unregisterJob(ctx,job);
            freeJob(job);
            known++;
        }
    }
    return RedisModule_ReplyWithLongLong(ctx,known);
}
