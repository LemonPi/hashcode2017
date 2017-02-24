# hashcode2017

## Algorithm
Each cache has a score for all videos that could potentially be
requested from it. This score is the sum of all time saved by storing
this video here. We then greedily select the video and cache pair that
yields the most savings and store the video there. 


To propagate this
selection, consider all possible effected caches. These are all the
caches connected to all the endpoints that are connected to the cache
that was just selected. Readjust their video score for this video from
the connected endpoint such that the time saving is considered relative
to the selected cache rather than data center.

This cache-video selection occurs until all caches are filled.
