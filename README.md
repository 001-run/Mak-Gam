Mak-Gam
=======
A fledgling, wanna-be-real-game-but-currently-just-a-prototype, game-engine.
This project's design is very much inspired by the Handmade Hero series, namely
coding style and system architecture (Including probably the greatest
architectural desicion ever, loading the game code in as a dll to the platform
code).  
  
In addition to hot code-reloading, this project explores circle-circle
collision, circle-line collision, and my personal favorite and proud discovery,
_extreme analogstick-sensitivity_. 

![Pushy pushy](http://i.imgur.com/l9mDupK.gif)

!Note that a control is required to test the demo.

Installation
------------

A pre-built binary is available within the build folder.
```
./build/sdl_main.exe
```

To build a new binary on a Windows environment, first run the appropriate 32-bit
vcvarsall.bat, then `./build.bat`.

Usage
-----

| Controls | Description |
| :---: | --- |
| Left AnalogStick | Movement |

Note that even super-slight movement can change the player avatar's direction.  
Through attempts in debugging and refining player control, I found that values
in the deadzone are still reliable to define direction. In fact, deadzone values
are actually pretty reliable in terms of precision. It's just that the physical
analogstick itself can not reliably return to a (0,0) position. Further research
can probably be put into expanding the usage of the deadzone, but I find that
limiting it to directional data is sufficiently useful in most cases, as the
actual usability of analog sticks within the deadzone region is pretty finicky.
Having a chance to rest the thumb is a good idea.

Motivation
----------
You know how they tell you that you shouldn't make a big game in C as your first
project? Well, I certainly didn't listen. Although not technically my first
endeavour in vidya games, Mak-Gam is the culmination of my fervorous efforts to
relearn programming after some serious mental health degredation earlier that
year (2015). I would not have dared carry that fervor were it not for my very
dear friend [Hurshal Patel](https://github.com/choochootrain). I would not have
dared push myself into such an arduous project were it not for [Recurse
Center](https://www.recurse.com/). And, I would not have dared to re-evaluate how
I code were it not for [Casey
Muratori](https://mollyrocket.com/casey/about.html).

yay  
