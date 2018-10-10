# Usage:
To run the program, please change directory
to `./debug`, where all the make files are
located
`$ cd debug`

Then `$ make`

After the objects are compiled, run 
`$ prj1_tm -p port -h hostfile -c count`
Note that port number should be of range
10000 to 65000

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
