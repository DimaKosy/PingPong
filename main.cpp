#include <iostream>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <SDL2/SDL.h>

#define NUM_THREADS 5
#define X_SCALE 16
#define Y_SCALE 9

using namespace std;

class Screen{
    private:
        int scaler = 50;                        //screen scaler

    public:
        int Width = X_SCALE*scaler;                  //screen width
        int Height = Y_SCALE*scaler;                  //screen height
        int ScreenX[2], ScreenY[2];             //dynamic w/h varibles

        SDL_Window * window = NULL;
        SDL_Renderer *render;
        SDL_Rect Viewport={0,0,Width,Height};   //for maintaining a fixed aspect
        
    //Initialises the screen and renderer
    bool init(){
        SDL_Init(SDL_INIT_EVERYTHING);
        window = SDL_CreateWindow( "PingPong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Width, Height, SDL_WINDOW_RESIZABLE);
        if(NULL == window){     //Check to see if window creation failed
            cout << "Could not create window: " << SDL_GetError() << endl;
            return false;       //Safe Exit
        }

        render = SDL_CreateRenderer(window, -1, 0);
        SDL_SetRenderDrawBlendMode(render,SDL_BLENDMODE_BLEND);
        SDL_GetWindowSize(window,&ScreenX[0],&ScreenY[0]);  //Assignes default sizes for screen comparison
        return true;
    }

    //Resets renderer on screen change
    void ResetRender(){
        SDL_GetWindowSize(window,&ScreenX[0],&ScreenY[0]);      //Assignes sizes for screen comparison

        if(ScreenX[1] == ScreenX[0] && ScreenY[1] == ScreenY[0]){   //Compares sizes of screen since last change
            return;     //returns if no change in size
        }

        ScreenX[1] = ScreenX[0];    //Changes comparison variable for width
        ScreenY[1] = ScreenY[0];    //Changes comparison variable for height

        SDL_DestroyRenderer(render);                //destroys old renderer instance
        render = SDL_CreateRenderer(window, -1, 0); //creates new renderer instance
        SDL_SetRenderDrawBlendMode(render,SDL_BLENDMODE_BLEND);
        
        //View Scaling
        if(ScreenX[0]/X_SCALE < ScreenY[0]/Y_SCALE){   //Checks what ratio is smaller
            Viewport.w = ScreenX[0];
            Viewport.h = Y_SCALE*((float)ScreenX[0]/X_SCALE);
        }
        else{
            Viewport.w = X_SCALE*((float)ScreenY[0]/Y_SCALE);
            Viewport.h = ScreenY[0];
        }

        //*DEBUG*/  printf("X:%d Y:%d\n",ScreenX[0],ScreenY[0]);
        //*DEBUG*/  printf("Scale %f %f\n",(float)Viewport.w/(X_SCALE*scaler),(float)Viewport.h/(Y_SCALE*scaler));
        
        SDL_RenderSetLogicalSize(render,Viewport.w,Viewport.h);                                             //centeres viewport
        SDL_RenderSetScale(render,(float)Viewport.w/(X_SCALE*scaler), (float)Viewport.h/(Y_SCALE*scaler));  //sets view scale
    }

    //Exits and destroys renderer and window references
    void exit(){
        SDL_DestroyRenderer(render);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

class Ball{
    public:
        SDL_FRect Body;

    //Moves the Rect
    void Move(float x, float y){
        Body.x += x;
        Body.y += y;
    }

    //Renders the Rect
    void Render(SDL_Renderer * renderer, char r, char g, char b,char a){
        SDL_SetRenderDrawColor(renderer,r,g,b,a);
        SDL_RenderFillRectF(renderer, &Body);

    }
};

class Bat{
    public:
        float BatSpeed = 1.5;
        SDL_FRect Body = {
            10,0,
            20,100
        };
        float CompErr = 3;
        float CompErrDist = 0.5;
        float CompErrMult = 3;

    void Move(int x, int y){
        Body.x += x;
        Body.y += y;
        //printf("Move");
    }

    void CompController(int MaxWidth,int MaxHeight, SDL_FRect Ball){
        float Dist = abs((Body.x+Body.w/2) - (Ball.x+Ball.w/2));
        float Target;

        Target = (MaxHeight/2) + ((pow(MaxWidth*CompErrDist,CompErrMult) - pow(Dist,CompErrMult))/pow(MaxWidth*CompErrDist,CompErrMult)) * ((Ball.y + (Ball.h/2)) - (MaxHeight/2));

        if(Target < Body.y + (Body.h/2) - CompErr*BatSpeed){
            Move(0, -BatSpeed);

            if (Body.y < 0){
                Body.y = 0;
            }
            return;
        }
        if(Target > Body.y + (Body.h/2) + CompErr*BatSpeed){
            Move(0, BatSpeed);

            if(Body.y >= MaxHeight - Body.h){
                Body.y = MaxHeight - Body.h;
            }
            return;
        }
    }

    void Render(SDL_Renderer * renderer, int r, int g, int b){
        SDL_SetRenderDrawColor(renderer,r,g,b,255);
        SDL_RenderFillRectF(renderer, &Body);
        //SDL_RenderDrawRectF(renderer, &Body);
    }

};

typedef struct Source{
    //Program Variables
    bool EventJoined = false;       //Check if Event Thread has joined
    bool ControlJoined = false;     //Check if Game Thread has joined
    bool Running;                   //Checks if Program is still running
    int RefreshRate = 120;          //Screen Refresh rate
    int FrameRate = 120;            //Game Refrash rate
    int Score[2] = {0,0};           //Score Variable
    pthread_t threads[NUM_THREADS]; //Pointers to threads
    
    //Program Objects
    Screen * Screen1;
    Ball Ball1;
    Bat bat[2];
}Source;

//Game Thread
void * GameThread(void * PassedStruct){
    printf("Entered GameThread\n");
    //Engine Variables
    Source * S_List = (Source *)(PassedStruct);   //Translating passed void * to Object *
    const Uint8 *state = SDL_GetKeyboardState(NULL);    //Creating Keyboard state stack
    clock_t START, CUR;                                 //For time comparison
    int TimeDif;                                        //Time since last frame
    float FRAMESPEED = 60;

    //Game Variables
    float DirX = 1;
    float DirY = 1;
    float SpeedX = FRAMESPEED;
    float SpeedY = FRAMESPEED;
    
    SpeedX /= S_List->FrameRate;
    SpeedY /= S_List->FrameRate;
    S_List->bat[0].BatSpeed = 6*(FRAMESPEED/S_List->FrameRate);
    S_List->bat[1].BatSpeed = 6*(FRAMESPEED/S_List->FrameRate);

    while(!S_List->EventJoined);    //Thread sync
    if(!S_List->Running){
        return 0;
    }

    //Just for Comp
    SpeedX = 4*FRAMESPEED/S_List->FrameRate;
    SpeedY = 4*FRAMESPEED/S_List->FrameRate;
    //END

    while(S_List->Running){
        CUR = clock();
        TimeDif = CUR - START;

        if(TimeDif >= (1000/S_List->FrameRate)){
            START = clock();

            //Game Code
            if(S_List->Ball1.Body.x <= 0){
                DirX = 1;
                S_List->Score[1]++;

                S_List->Ball1.Body={S_List->bat[0].Body.x + S_List->Ball1.Body.w,S_List->bat[0].Body.y + S_List->bat[0].Body.h/2,20,20};

                printf("%d:%d  Speed:%f\n",S_List->Score[0],S_List->Score[1],SpeedX);
                
                SpeedX = FRAMESPEED/S_List->FrameRate + (SpeedX-FRAMESPEED/S_List->FrameRate)/2;
                SpeedY = FRAMESPEED/S_List->FrameRate + (SpeedX-FRAMESPEED/S_List->FrameRate)/2;
                
            }
            else if(S_List->Ball1.Body.x + S_List->Ball1.Body.w >= S_List->Screen1->Width){
                DirX = -1;
                S_List->Score[0]++;

                S_List->Ball1.Body={S_List->bat[1].Body.x - S_List->Ball1.Body.w,S_List->bat[1].Body.y + S_List->bat[1].Body.h/2,20,20};

                printf("%d:%d  Speed:%f\n",S_List->Score[0],S_List->Score[1],SpeedX);
                
                SpeedX = FRAMESPEED/S_List->FrameRate + (SpeedX-FRAMESPEED/S_List->FrameRate)/2;
                SpeedY = FRAMESPEED/S_List->FrameRate + (SpeedX-FRAMESPEED/S_List->FrameRate)/2;
                
            }

            if(SDL_HasIntersectionF(&S_List->bat[0].Body,&S_List->Ball1.Body) == SDL_TRUE){
                /*
                SpeedX += 10.0/S_List->FrameRate;
                SpeedY += 8.0/S_List->FrameRate;
                */
                DirX = 1;
            }
            else if(SDL_HasIntersectionF(&S_List->bat[1].Body,&S_List->Ball1.Body) == SDL_TRUE){
                /*
                SpeedX += 10.0/S_List->FrameRate;
                SpeedY += 8.0/S_List->FrameRate;
                */
                DirX = -1;
            }

            if(S_List->Ball1.Body.y < 0){
                DirY = 1;
            }
            else if(S_List->Ball1.Body.y + S_List->Ball1.Body.h >= S_List->Screen1->Height){
                DirY = -1;
            }

            //Player Bat Movement
            if(state[SDL_SCANCODE_W]){
                S_List->bat[0].Move(0, -S_List->bat[0].BatSpeed);

                if (S_List->bat[0].Body.y < 0){
                    S_List->bat[0].Body.y = 0;
                }
            }
            if(state[SDL_SCANCODE_S]){
                S_List->bat[0].Move(0, S_List->bat[0].BatSpeed);

                if(S_List->bat[0].Body.y > S_List->Screen1->Height - S_List->bat[0].Body.h){
                    S_List->bat[0].Body.y = S_List->Screen1->Height - S_List->bat[0].Body.h;
                }
            }

            //Comp Bat Movement
            S_List->bat[1].CompController(S_List->Screen1->Width,S_List->Screen1->Height,S_List->Ball1.Body);
            S_List->bat[0].CompController(S_List->Screen1->Width,S_List->Screen1->Height,S_List->Ball1.Body);

            S_List->Ball1.Move(SpeedX*DirX*3,SpeedY*DirY);

            //End of Game Code
        }
    }

    printf("Exit Game thread\n");
    pthread_exit(NULL);
    return NULL;
}

//Screen Event Thread
void * EventThread(void * PassedStruct){
    printf("Entered EventThread\n");

    static Screen MainWindow;                           //initialize Mainwindow object
    SDL_Event event;                                    //event stack

    Source * S_List = (Source *)(PassedStruct);   //Translating passed void * to Object *

    S_List->Screen1 = &MainWindow;                  //passing Mainwindow address to global address struct

    if(!MainWindow.init()){
        S_List->Running = false;
        S_List->EventJoined = true;
        return NULL;
    }

    S_List->Running = true;
    S_List->EventJoined = true;
    
    while(S_List->Running){
        while(SDL_PollEvent(&event)){
            if (SDL_QUIT == event.type){
                S_List->Running = false;
                break;
            }    
            
            /*switch(event.type){
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                    case SDL_WINDOWEVENT_SHOWN:
                        SDL_Log("Window %d shown", event.window.windowID);
                        break;
                    case SDL_WINDOWEVENT_HIDDEN:
                        SDL_Log("Window %d hidden", event.window.windowID);
                        break;
                    case SDL_WINDOWEVENT_EXPOSED:
                        SDL_Log("Window %d exposed", event.window.windowID);
                        break;
                    case SDL_WINDOWEVENT_MOVED:
                        SDL_Log("Window %d moved to %d,%d",
                                event.window.windowID, event.window.data1,
                                event.window.data2);
                        break;
                    case SDL_WINDOWEVENT_RESIZED:
                        SDL_Log("Window %d resized to %dx%d",
                                event.window.windowID, event.window.data1,
                                event.window.data2);
                        break;
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        SDL_Log("Window %d size changed to %dx%d",
                                event.window.windowID, event.window.data1,
                                event.window.data2);
                        break;
                    case SDL_WINDOWEVENT_MINIMIZED:
                        SDL_Log("Window %d minimized", event.window.windowID);
                        break;
                    case SDL_WINDOWEVENT_MAXIMIZED:
                        SDL_Log("Window %d maximized", event.window.windowID);
                        break;
                    case SDL_WINDOWEVENT_RESTORED:
                        SDL_Log("Window %d restored", event.window.windowID);
                        break;
                    case SDL_WINDOWEVENT_ENTER:
                        SDL_Log("Mouse entered window %d",
                                event.window.windowID);
                        break;
                    case SDL_WINDOWEVENT_LEAVE:
                        SDL_Log("Mouse left window %d", event.window.windowID);
                        break;
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        SDL_Log("Window %d gained keyboard focus",
                                event.window.windowID);
                        break;
                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        SDL_Log("Window %d lost keyboard focus",
                                event.window.windowID);
                        break;
                    case SDL_WINDOWEVENT_CLOSE:
                        SDL_Log("Window %d closed", event.window.windowID);
                        break;
                    #if SDL_VERSION_ATLEAST(2, 0, 5)
                        case SDL_WINDOWEVENT_TAKE_FOCUS:
                            SDL_Log("Window %d is offered a focus", event.window.windowID);
                            break;
                        case SDL_WINDOWEVENT_HIT_TEST:
                            SDL_Log("Window %d has a special hit test", event.window.windowID);
                            break;
                    #endif
                    default:
                        SDL_Log("Window %d got unknown event %d",
                                event.window.windowID, event.window.event);
                        break;
                    }
                break;
            }*/
            
        }
    }

    printf("Exit EventThread\n");
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char *argv[]){
    printf("Entered Main\n");

    Source S_List_M;
    clock_t START, CUR;

    int TimeDif;
    int Err;

    Err = pthread_create(&S_List_M.threads[1], NULL, GameThread, (void *)(&S_List_M));
    if (Err != 0) {
        cout << "Error:unable to create Control thread" << endl;

        S_List_M.EventJoined = true;
        S_List_M.Running = false;
    }
    Err = pthread_create(&S_List_M.threads[0], NULL, EventThread, (void *)(&S_List_M));
    if (Err != 0) {
        cout << "Error:unable to create Event thread" << endl;

        S_List_M.EventJoined = true;
        S_List_M.Running = false;
    }

    while(!S_List_M.EventJoined);   //Thread sync
    if(!S_List_M.Running){
        return 0;
    }

    SDL_Rect RenderBox={
        0,0,
        S_List_M.Screen1->Width, S_List_M.Screen1->Height
    };

    S_List_M.Ball1.Body={(float)S_List_M.Screen1->ScreenX[0]/2,0,20,20};

    SDL_SetRenderDrawColor(S_List_M.Screen1->render,0,0,0,255);
    SDL_RenderClear(S_List_M.Screen1->render);
    SDL_RenderPresent(S_List_M.Screen1->render);

    S_List_M.bat[1].Body.x = S_List_M.Screen1->ScreenX[0] - S_List_M.bat[1].Body.w-10;

    START = clock();
    while(S_List_M.Running){       

        CUR = clock();
        TimeDif = CUR - START;

        if(TimeDif >= (1000/S_List_M.RefreshRate)){
            START = clock();

            S_List_M.Screen1->ResetRender();

            //printf("Ran");
            SDL_SetRenderDrawColor(S_List_M.Screen1->render,0,0,0,255);
            SDL_RenderClear(S_List_M.Screen1->render);

            S_List_M.Ball1.Render(S_List_M.Screen1->render,255,255,255,255);
            S_List_M.bat[0].Render(S_List_M.Screen1->render,0,0,255);
            S_List_M.bat[1].Render(S_List_M.Screen1->render,255,0,0);

            SDL_SetRenderDrawColor(S_List_M.Screen1->render,255,255,255,255);
            SDL_RenderDrawRect(S_List_M.Screen1->render, &RenderBox);

            SDL_RenderPresent(S_List_M.Screen1->render);    //Push to window
        }
    }

    //EXIT
    printf("Closing\n");
    S_List_M.Screen1->exit();
    pthread_join(S_List_M.threads[1],NULL);
    pthread_join(S_List_M.threads[0],NULL);
    return EXIT_SUCCESS;

}