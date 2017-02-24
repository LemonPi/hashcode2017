#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace std;

using video_size_t  = int;
using video_id_t    = size_t;
using cache_id_t    = size_t;
using endpoint_id_t = size_t;
using request_id_t  = size_t;
using latency_t     = int;
using score_t       = size_t;

struct Cache {
    map<video_id_t, map<endpoint_id_t, score_t>> videoScores;
    map<video_id_t, map<endpoint_id_t, score_t>> oldVideoScores;

    set<endpoint_id_t> connectedEndpoints;
    set<video_id_t> storedVideos;

    size_t spaceLeft;
};

struct Request {
    size_t repeat;
    video_id_t video;
    endpoint_id_t endpoint;
};

struct Endpoint {
    latency_t LD;
    map<cache_id_t, latency_t> connectedCaches;
    map<video_id_t, Request> requests;
};

void my_handler(int s) {
}

int main(int argc, char* argv[]) {
    size_t numVideos, numEndpoints, numRequests, numCache, cacheSize;
    cin >> numVideos >> numEndpoints >> numRequests >> numCache >> cacheSize;

    vector<video_size_t> videoSizes(numVideos);
    vector<Endpoint> endpoints(numEndpoints);
    vector<Request> requests(numRequests);
    vector<Cache> caches(numCache);

    // CACHES
    for (cache_id_t c = 0; c < numCache; ++c) {
        caches[c].spaceLeft = cacheSize;
    }

    // VIDEOS
    for (video_id_t v = 0; v < numVideos; ++v) {
        video_size_t size;
        cin >> size;
        videoSizes[v] = size;
    }

    // ENDPOINTS
    for (endpoint_id_t e = 0; e < numEndpoints; ++e) {
        auto& endpoint = endpoints[e];
        cin >> endpoint.LD;

        int numCache;
        cin >> numCache;

        for (int i = 0; i < numCache; ++i) {
            cache_id_t cache;
            cin >> cache;
            latency_t LC;
            cin >> LC;
            endpoint.connectedCaches.emplace(cache, LC);
            caches[cache].connectedEndpoints.emplace(e);
        }
    }

    // REQUESTS
    for (request_id_t r = 0; r < numRequests; ++r) {
        auto& request = requests[r];
        cin >> request.video >> request.endpoint >> request.repeat;
        auto& endpoint = endpoints[request.endpoint];

        if (endpoint.requests.find(request.video) == endpoint.requests.end()) {
            endpoint.requests[request.video] = request;
        } else {
            endpoint.requests[request.video].repeat += request.repeat;
        }
    }

    // CALCULATE initial value
    for (endpoint_id_t e = 0; e < endpoints.size(); ++e) {
        for (auto& cacheEdge : endpoints[e].connectedCaches) {
            cache_id_t c = cacheEdge.first;
            latency_t LC = cacheEdge.second;

            for (auto& requestPair : endpoints[e].requests) {
                video_id_t video = requestPair.first;
                // compared to data center how much we save
                auto save = (endpoints[e].LD - LC) * requestPair.second.repeat;
                caches[c].videoScores[video][e] += save;
            }
        }
    }

    // FIND best cache and video to put in
    cache_id_t bestCache = 0;
    video_id_t bestVideo = 0;
    score_t bestScore    = 0;
    int itr              = 0;
    while (true) {
        bestScore = 0;
        for (cache_id_t c = 0; c < caches.size(); ++c) {
            for (const auto& videoPair : caches[c].videoScores) {
                video_id_t v  = videoPair.first;
                score_t score = 0;
                for (const auto& connectionPair : videoPair.second) {
                    score += connectionPair.second;
                }

                if (score > bestScore && videoSizes[v] <= caches[c].spaceLeft) {
                    bestCache = c;
                    bestScore = score;
                    bestVideo = v;
                }
            }
        }
        if (bestScore == 0) {
            break;
        }

        // PUT video in cache
        auto& bc = caches[bestCache];
        bc.storedVideos.emplace(bestVideo);
        bc.spaceLeft -= videoSizes[bestVideo];
        bc.oldVideoScores[bestVideo] = bc.videoScores[bestVideo];
        for (auto& connectionPair : bc.videoScores[bestVideo]) {
            // for every endpoint with this video, we 0 it to avoid making the same put choice
            connectionPair.second = 0;
        }

        // RECALCULATE costs
        for (endpoint_id_t e : bc.connectedEndpoints) {
            auto request = endpoints[e].requests.find(bestVideo);
            if (request != endpoints[e].requests.end()) {
                auto puttedCacheLatency = endpoints[e].connectedCaches[bestCache];
                for (auto& connectedCachePair : endpoints[e].connectedCaches) {
                    auto c = connectedCachePair.first;
                    // skip self
                    if (c == bestCache) {
                        continue;
                    }

                    latency_t thisLatency = connectedCachePair.second;

                    if (puttedCacheLatency <= thisLatency) {
                        caches[c].videoScores[bestVideo][e] = 0;
                    } else {
                        caches[c].videoScores[bestVideo][e] =
                            (puttedCacheLatency - thisLatency) * request->second.repeat;
                    }
                }
            }
        }
        ++itr;

        cout << "*********************************************************************" << endl;
        // OUTPUT
        // assign
        cout << numCache << endl;
        for (cache_id_t c = 0; c < caches.size(); ++c) {
            cout << c << " ";
            for (const auto& v : caches[c].storedVideos) {
                cout << v << " ";
            }
            cout << endl;
        }
    }

    return 0;
}
