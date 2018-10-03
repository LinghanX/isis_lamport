# Tasks at hand:

* Need a script file to dictate starting/sending/error
detection etc.

* Multibroadcast 

* Error detection on receiving messages from `main()`


references:

https://www.youtube.com/watch?v=yHRYetSvyjU

# FSM legend:

| state id | description |
| --- | --- |
| init | init state: finished all memory relocation | 
| 1 | idle: nothing to do | 
| 2 | waiting for all ackMsg| 
| 3 | send DataMsg to all p that has no ackMsg|
| (1) | send back ackMsg| 
| (2) | drop it to the floor | 
