# DISRUPTOR

## ABOUT

* An FPS game where you throw your pet robot, "Bug" to cause your mechanical foes to malfunction.
  Built entirely with a custom engine using Raylib and heavily inspired by id Software's id Tech II.

<img src="repo_clip.gif" width="1280"/> 


## CONTROLS

* look/aim:           mouse move
* move:               WASD
* shoot/throw:        mouse left
* shoot alt:          mouse right
* next weapon:        E, scroll up
* previous weapon:    Q, scroll down
* exit game:          ESC


## COMPILATION

* Clone and navigate to repo
``` bash
git clone https://github.com/yaskai/disruptor.git .
cd ./disruptor
```

### LINUX

* Build libraries
``` bash
cd ./build/external/raylib/src
make
```

* Build game
``` bash
# return to project root directory
cd ../../../..
make
```

* Run game
``` bash
./bin/game
```

### WINDOWS

* Easiest method for compiling on Windows is through w64devkit, available at https://github.com/skeeto/w64devkit/releases: 

* Launch w64devkit

* Build libraries
``` bash
cd ./build/external/raylib/src
make
```

* Build game
``` bash
# return to project root directory
cd ../../../..
make
  
* Run game
``` bash
./game.exe

