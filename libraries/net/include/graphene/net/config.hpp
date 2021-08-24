/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#define GRAPHENE_NET_PROTOCOL_VERSION                        106

/**
 * Define this to enable debugging code in the p2p network interface.
 * This is code that would never be executed in normal operation, but is
 * used for automated testing (creating artificial net splits,
 * tracking where messages came from and when)
 */
#define ENABLE_P2P_DEBUGGING_API                             1

#define GRAPHENE_NET_DEFAULT_PEER_CONNECTION_RETRY_TIME      30 // seconds

/**
 * AFter trying all peers, how long to wait before we check to
 * see if there are peers we can try again.
 */
#define GRAPHENE_PEER_DATABASE_RETRY_DELAY                   15 // seconds

#define GRAPHENE_NET_PEER_HANDSHAKE_INACTIVITY_TIMEOUT       5

#define GRAPHENE_NET_PEER_DISCONNECT_TIMEOUT                 20

#define GRAPHENE_NET_TEST_P2P_PORT                           1700
#define GRAPHENE_NET_DEFAULT_P2P_PORT                        1776
#define GRAPHENE_NET_DEFAULT_DESIRED_CONNECTIONS             20
#define GRAPHENE_NET_DEFAULT_MAX_CONNECTIONS                 200

#define GRAPHENE_NET_MAXIMUM_QUEUED_MESSAGES_IN_BYTES        (1024 * 1024)

/**
 * When we receive a message from the network, we advertise it to
 * our peers and save a copy in a cache were we will find it if
 * a peer requests it.  We expire out old items out of the cache
 * after this number of blocks go by.
 * 
 * Recently lowered from 30 to match the default expiration time
 * the web wallet imposes on transactions.
 */
#define GRAPHENE_NET_MESSAGE_CACHE_DURATION_IN_BLOCKS        5

/**
 * We prevent a peer from offering us a list of blocks which, if we fetched them
 * all, would result in a blockchain that extended into the future.
 * This parameter gives us some wiggle room, allowing a peer to give us blocks
 * that would put our blockchain up to an hour in the future, just in case
 * our clock is a bit off.
 */
#define GRAPHENE_NET_FUTURE_SYNC_BLOCKS_GRACE_PERIOD_SEC     (60 * 60)

#define GRAPHENE_NET_MAX_INVENTORY_SIZE_IN_MINUTES           2

#define GRAPHENE_NET_MAX_BLOCKS_PER_PEER_DURING_SYNCING      200

/**
 * During normal operation, how many items will be fetched from each
 * peer at a time.  This will only come into play when the network
 * is being flooded -- typically transactions will be fetched as soon
 * as we find out about them, so only one item will be requested
 * at a time.
 * 
 * No tests have been done to find the optimal value for this
 * parameter, so consider increasing or decreasing it if performance
 * during flooding is lacking.
 */
#define GRAPHENE_NET_MAX_ITEMS_PER_PEER_DURING_NORMAL_OPERATION  1000 

/**
 * Instead of fetching all item IDs from a peer, then fetching all blocks
 * from a peer, we will interleave them.  Fetch at least this many block IDs,
 * then switch into block-fetching mode until the number of blocks we know about
 * but haven't yet fetched drops below this
 */
#define GRAPHENE_NET_MIN_BLOCK_IDS_TO_PREFETCH               10000

#define GRAPHENE_NET_MAX_TRX_PER_SECOND                      2000
