#include <stdio.h>
#include <stdbool.h>
#include <windows.h>
#include <stdint.h>
#include <math.h>
#include "SDL.h"

#define internal static
#define global_variable static
#define local_persist static

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 270

#define CLIP(X, A, B) ((X < A) ? A : ((X > B) ? B : X))
#define SQRT2 1.41421356237
#define ABS(X) (X < 0 ? -X : X)

global_variable bool GlobalRunning = true;

global_variable SDL_Window *GlobalWindow;
global_variable SDL_Renderer *GlobalRenderer;

global_variable float GlobalDt;

struct vector
{
	float x;
	float y;
};

internal float
GetDistanceBetweenPoints(float X1, float Y1, float X2, float Y2)
{
	float X = X2 - X1;
	float Y = Y2 - Y1;
	return sqrt(X * X + Y * Y);
}

internal float
GetAngleBetweenPoints(float X1, float Y1, float X2, float Y2)
{
	float Y = Y2 - Y1;
	float X = X2 - X1;

	return atan2(Y, X) * 180 / 3.14;
}

struct button_state
{
	bool IsDownLastState;
	uint32_t Duration;
	bool IsDown;
};

// NOTE(sigmasleep): There needs to be a threshold for determining a change in direction.

struct controller_state
{
	struct
	{
		float XLastState;
		float YLastState;
		uint32_t Duration;
		float X;
		float Y;
	};

	union
	{
		button_state Buttons[4];
		struct
		{
			button_state Up;
			button_state Down;
			button_state Left;
			button_state Right;
		};
	};
};

struct input_state
{
	controller_state Controllers[4];
};

struct baddie
{
	vector Position;
	float Radius;
	float Angle;
};

struct hero
{
	vector Position;
	int CurrentPathIndex;
	float DirectionFacing;
	float Radius;
	float HalfHeight;
};

struct scene
{
	hero Hero;
	baddie Baddies[255];
	int BaddieCount;
};

internal void
AddBaddieToScene(baddie *Baddie, scene *Scene)
{
	Scene->Baddies[Scene->BaddieCount].Angle = Baddie->Angle;
	Scene->Baddies[Scene->BaddieCount].Position = Baddie->Position;
	Scene->Baddies[Scene->BaddieCount].Radius = Baddie->Radius;
	Scene->BaddieCount++;
}

internal void
BaddieMovement(baddie *Baddie)
{
	float Distance = 10 * GlobalDt;

	Baddie->Position.x += cos(Baddie->Position.y/10) * Distance;
	Baddie->Position.y += sin(Baddie->Position.x/10) * Distance;
}

internal void
DrawTriangle(int X, int Y, float Angle, int HalfHeight)
{
	SDL_Point Points[4];
	Points[0].x = X + cos(Angle * 3.14 / 180.f) * HalfHeight;
	Points[0].y = Y + sin(Angle * 3.14 / 180.f) * HalfHeight;
	Points[3].x = Points[0].x;
	Points[3].y = Points[0].y;

	Points[1].x = X + cos((Angle + 120) * 3.14 / 180.f) * HalfHeight;
	Points[1].y = Y + sin((Angle + 120) * 3.14 / 180.f) * HalfHeight;

	Points[2].x = X + cos((Angle - 120) * 3.14 / 180.f) * HalfHeight;
	Points[2].y = Y + sin((Angle - 120) * 3.14 / 180.f) * HalfHeight;

	SDL_RenderDrawLines(GlobalRenderer, Points, 4);
}


internal void
DrawSemiCircle(
	float X, float Y,
	float Radius,
	float Segments, float TotalSegments,
	float Angle)
{
	float RadAngle = Angle * 3.14 / 180;

	vector Position1;
	vector Position2;
	Position1.x = X + cos(RadAngle) * Radius;
	Position1.y = Y + sin(RadAngle) * Radius;

	for(int PointNum = 0; PointNum < Segments; PointNum++)
	{
		Position2.x = X + cos(RadAngle + PointNum / TotalSegments * 3.14 * 2) * Radius;
		Position2.y = Y + sin(RadAngle + PointNum / TotalSegments * 3.14 * 2) * Radius;
		SDL_RenderDrawLine(GlobalRenderer,
						   Position1.x, Position1.y,
						   Position2.x, Position2.y);
		Position1.x = Position2.x;
		Position1.y = Position2.y;
	}

	Position2.x = X + cos(RadAngle) * Radius;
	Position2.y = Y + sin(RadAngle) * Radius;
	SDL_RenderDrawLine(GlobalRenderer,
					   Position1.x, Position1.y,
					   Position2.x, Position2.y);
}

internal void
DrawCircle(float X, float Y, float Radius, float Segments)
{
	DrawSemiCircle(X, Y, Radius, Segments, Segments, 0);
}

internal void
DrawBox(float X, float Y, float Width, float Height)
{
	SDL_Rect FillRect;
	FillRect.x = (int)X;
	FillRect.y = (int)Y;
	FillRect.w = (int)Width;
	FillRect.h = (int)Height;

	SDL_RenderDrawRect(GlobalRenderer, &FillRect);
}

internal void
RenderBaddie(baddie *Baddie)
{
	DrawSemiCircle(Baddie->Position.x, Baddie->Position.y,
				   Baddie->Radius,
				   16, 32,
				   Baddie->Angle);
	DrawSemiCircle(Baddie->Position.x, Baddie->Position.y,
				   Baddie->Radius,
				   16, 32,
				   Baddie->Angle + 180);
}

internal void
RunOnBaddiesInScene(scene *Scene, void (*BaddieFunction)(baddie*))
{
	for(int BaddieIndex = 0; BaddieIndex < Scene->BaddieCount; BaddieIndex++)
	{
		BaddieFunction(&(Scene->Baddies[BaddieIndex]));
	}
}

internal void
RenderScene(scene *Scene)
{
	RunOnBaddiesInScene(Scene, RenderBaddie);

	DrawTriangle(Scene->Hero.Position.x, Scene->Hero.Position.y,
				 Scene->Hero.DirectionFacing,
				 Scene->Hero.HalfHeight);
	DrawCircle(Scene->Hero.Position.x, Scene->Hero.Position.y, Scene->Hero.Radius, 32);
//	DrawBox(Scene->Hero.Position.x - Scene->Hero.HalfHeight, Scene->Hero.Position.y - Scene->Hero.HalfHeight,
//			Scene->Hero.HalfHeight * 2, Scene->Hero.HalfHeight * 2);
}

internal void
CollideWithBaddie(hero *Hero, baddie *Baddie)
{
	float Distance = GetDistanceBetweenPoints(
		Hero->Position.x, Hero->Position.y,
		Baddie->Position.x, Baddie->Position.y
	);

	if(Distance < Hero->Radius + Baddie->Radius)
	{
		float PushDistance = Baddie->Radius - (Distance - Hero->Radius);
		float Direction = GetAngleBetweenPoints(
			Hero->Position.x, Hero->Position.y,
			Baddie->Position.x, Baddie->Position.y);
		Baddie->Position.x += cos(Direction * 3.14 / 180.f) * PushDistance;
		Baddie->Position.y += sin(Direction * 3.14 / 180.f) * PushDistance;
	}

	float x = Hero->Position.x + cos(Hero->DirectionFacing * 3.14 / 180.f) * Hero->HalfHeight;
	float y = Hero->Position.y + sin(Hero->DirectionFacing * 3.14 / 180.f) * Hero->HalfHeight;

	Distance = GetDistanceBetweenPoints(
		x, y,
		Baddie->Position.x, Baddie->Position.y
	);

	if(Distance < Baddie->Radius)
	{
		float Direction = GetAngleBetweenPoints(
			x, y,
			Baddie->Position.x, Baddie->Position.y
		);

		Baddie->Angle = Direction;
	}
}

internal void
ProcessControllerMovement(controller_state *Controller, vector *Movement)
{
	float x = 0;
	float y = 0;

	if(Controller->Left.IsDown)
	{
		x -= 1;
	}
	if(Controller->Right.IsDown)
	{
		x += 1;
	}
	if(Controller->Up.IsDown)
	{
		y -= 1;
	}
	if(Controller->Down.IsDown)
	{
		y += 1;
	}

	if(x != 0 && y != 0)
	{
		x /= SQRT2;
		y /= SQRT2;
	}

	x += Controller->X;
	y += Controller->Y;

	Movement->x = CLIP(x, -1.f, 1.f);
	Movement->y = CLIP(y, -1.f, 1.f);
}

internal void
MovePlayer(hero *Hero, input_state *Input)
{
	vector InputMovement;

	ProcessControllerMovement(&Input->Controllers[0], &InputMovement);

	Hero->Position.x += 100 * InputMovement.x * GlobalDt;
	Hero->Position.y += 100 * InputMovement.y * GlobalDt;
}

// Note(sigmasleep): This should not have any calls to SDL in it
internal void
RenderGame(input_state *Input, scene *Scene)
{
	//NavigatePath();
	MovePlayer(&Scene->Hero, Input);
	RunOnBaddiesInScene(Scene, BaddieMovement);
	for(int BaddieIndex = 0; BaddieIndex < Scene->BaddieCount; BaddieIndex++)
	{
		CollideWithBaddie(&Scene->Hero, &Scene->Baddies[BaddieIndex]);
	}

	RenderScene(Scene);
}

int
main(int argc, char* args[])
{
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0)
	{
		return 1;
	}

	GlobalWindow = SDL_CreateWindow("SDL Test",
							  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							  SCREEN_WIDTH, SCREEN_HEIGHT,
							  0);
	
	GlobalRenderer = SDL_CreateRenderer(GlobalWindow, -1, 0);

	SDL_Event e;
	uint32_t lastTime = SDL_GetTicks();

	SDL_Rect fillRect = { SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4,
						SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };

	baddie Baddie = {};

	Baddie.Position.x = SCREEN_WIDTH / 2;
	Baddie.Position.y = SCREEN_HEIGHT / 2;
	Baddie.Radius = 14;

	scene Scene = {};

	AddBaddieToScene(&Baddie, &Scene);
	Baddie.Position.x += SCREEN_WIDTH / 4;
	AddBaddieToScene(&Baddie, &Scene);

	Scene.Hero.Position.x = SCREEN_WIDTH / 4;
	Scene.Hero.Position.y = SCREEN_HEIGHT / 4;
	Scene.Hero.DirectionFacing = 0;
	Scene.Hero.CurrentPathIndex = 0;
	Scene.Hero.Radius = 7;
	Scene.Hero.HalfHeight = acos(30 * 3.14 / 180) * Scene.Hero.Radius * 2;

	input_state Input = {};

	SDL_GameController *Controller1 = SDL_GameControllerOpen(0);
	while(GlobalRunning) {
		GlobalDt = (SDL_GetTicks() - lastTime)/1000.f;
		lastTime = SDL_GetTicks();
		
		while(SDL_PollEvent(&e) != 0)
		{
			switch(e.type)
			{
				case SDL_QUIT:
				{
					GlobalRunning = false;
				} break;
				case SDL_CONTROLLERAXISMOTION:
				{
					SDL_ControllerAxisEvent Event = e.caxis;

					// Note(sigmasleep): Normalization via division by int16 min/max values
					float Value = Event.value < 0 ? Event.value / 32768.f : Event.value / 32767.f;

					// Todo(sigmasleep): Move deadzone to a better place
					float Deadzone = 0.2;
					Value = ABS(Value) > Deadzone ? Value : 0;

					if(Event.which < 4)
					{
						controller_state* Controller = &Input.Controllers[Event.which];

						switch(Event.axis)
						{
							case SDL_CONTROLLER_AXIS_LEFTX:
							{
								Controller->XLastState = Controller->X;

								Controller->X = Value;
							} break;
							case SDL_CONTROLLER_AXIS_LEFTY:
							{
								Controller->YLastState = Controller->Y;

								Controller->Y = Value;
							} break;
						}
					}
				} break;
				case SDL_CONTROLLERBUTTONDOWN:
				case SDL_CONTROLLERBUTTONUP:
				{
					SDL_ControllerButtonEvent Event = e.cbutton;

					bool IsDown = Event.state == SDL_PRESSED;

					if(Event.which < 4)
					{
						controller_state* Controller = &Input.Controllers[Event.which];
						switch(Event.button)
						{
						case SDL_CONTROLLER_BUTTON_DPAD_UP:
						{
							Controller->Up.IsDownLastState = Controller->Up.IsDown;
							Controller->Up.IsDown = IsDown;
						} break;
						case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
						{
							Controller->Down.IsDownLastState = Controller->Down.IsDown;
							Controller->Down.IsDown = IsDown;
						} break;
						case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
						{
							Controller->Left.IsDownLastState = Controller->Left.IsDown;
							Controller->Left.IsDown = IsDown;
						} break;
						case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
						{
							Controller->Right.IsDownLastState = Controller->Right.IsDown;
							Controller->Right.IsDown = IsDown;
						} break;
						}
					}
				} break;
			}
		}

		SDL_SetRenderDrawColor(GlobalRenderer, 255, 0, 0, 255);
		SDL_RenderClear(GlobalRenderer);
		SDL_SetRenderDrawColor(GlobalRenderer, 0, 255, 0, 255);
		SDL_RenderFillRect(GlobalRenderer, &fillRect);

		SDL_SetRenderDrawColor(GlobalRenderer, 255, 255, 255, 255);

		SDL_SetRenderDrawColor(GlobalRenderer, 0, 0, 255, 255);
		RenderGame(&Input, &Scene);
		
		SDL_RenderPresent(GlobalRenderer);

		SDL_Delay(1);
	}

	SDL_GameControllerClose(Controller1);

	SDL_Quit();

	return(0);
}
