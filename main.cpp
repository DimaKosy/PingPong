#include <iostream>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <SDL2/SDL.h>

#define NUM_THREADS 5

using namespace std;

class Screen{
    private:
        int scaler = 50;
    public:
        int Width = 16*scaler;
        int Height = 9*scaler;
        int ScreenX[2], ScreenY[2];

        SDL_Window * window;
        SDL_Renderer *render;
        SDL_Rect Viewport={0,0,Width,Height};
        

    bool init(){
        SDL_Init(SDL_INIT_EVERYTHING);
        window = SDL_CreateWindow( "PingPong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, Width, Height, SDL_WINDOW_RESIZABLE);
        if ( NULL == window){
            cout << "Could not create window: " << SDL_GetError( ) << endl;
            return false;
        }

        render = SDL_CreateRenderer(window, -1, 0);
        SDL_GetWindowSize(window,&ScreenX[0],&ScreenY[0]);
        return true;
    }

    void ResetRender(){

        SDL_GetWindowSize(window,&ScreenX[0],&ScreenY[0]);

        if(ScreenX[1] == ScreenX[0] && ScreenY[1] == ScreenY[0]){
            return;
        }

        ScreenX[1] = ScreenX[0];
        ScreenY[1] = ScreenY[0];
        SDL_DestroyRenderer(render);
        render = SDL_CreateRenderer(window, -1, 0);
        
        if(ScreenX[0]/16.0 < ScreenY[0]/9.0){
            Viewport.w = ScreenX[0];
            Viewport.h = 9.0*(ScreenX[0]/16.0);
        }
        else{
            Viewport.w = 16.0*(ScreenY[0]/9.0);
            Viewport.h = ScreenY[0];
        }

        printf("X:%d Y:%d\n",ScreenX[0],ScreenY[0]);
        printf("Scale %f %f\n",Viewport.w/(16.0*scaler),Viewport.h/(9.0*scaler));
        
        SDL_RenderSetLogicalSize(render,Viewport.w,Viewport.h);
        SDL_RenderSetScale(render,Viewport.w/(16.0*scaler), Viewport.h/(9.0*scaler));
    }

    void exit(){
        SDL_DestroyRenderer(render);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

class Ball{
    public:
        SDL_FRect Body;

    void Move(float x, float y){
        Body.x += x;
        Body.y += y;
        //printf("Move");
    }

    void Render(SDL_Renderer * renderer, int r, int g, int b){
        SDL_SetRenderDrawColor(renderer,r,g,b,255);
        SDL_RenderFillRectF(renderer, &Body);
        //SDL_RenderDrawRectF(renderer, &Body);
    }
};

class Bat{
    public:
        float BatSpeed = 1.5;
        SDL_FRect Body = {
            10,0,
            20,100
        };

    void Move(int x, int y){
        Body.x += x;
        Body.y += y;
        //printf("Move");
    }

    void CompController(int MaxWidth,int MaxHeight, SDL_FRect Ball){
        float Dist = abs((Body.x+Body.w/2) - (Ball.x+Ball.w/2));
        float Target;

        Target = (MaxHeight/2) + ((pow(MaxWidth*1.1,2) - pow(Dist,2))/pow(MaxWidth*1.1,2)) * ((Ball.y + (Ball.h/2)) - (MaxHeight/2));

        if(Target < Body.y + (Body.h/2) - 1.1*BatSpeed){
            Move(0, -BatSpeed);

            if (Body.y < 0){
                Body.y = 0;
            }
            return;
        }
        if(Target > Body.y + (Body.h/2) + 1.1*BatSpeed){
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

typedef struct Objects{
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
}Objects;

//Game Thread
void * GameThread(void * PassedStruct){
    //Engine Variables
    Objects * ObjectList = (Objects *)(PassedStruct);   //Translating passed void * to Object *
    const Uint8 *state = SDL_GetKeyboardState(NULL);    //Creating Keyboard state stack
    clock_t START, CUR;                                 //For time comparison
    int TimeDif;                                        //Time since last frame
    float FRAMESPEED = 60;

    //Game Variables
    float DirX = 1;
    float DirY = 1;
    float SpeedX = FRAMESPEED;
    float SpeedY = FRAMESPEED;
    
    SpeedX /= ObjectList->FrameRate;
    SpeedY /= ObjectList->FrameRate;
    ObjectList->bat[0].BatSpeed = 6*(FRAMESPEED/ObjectList->FrameRate);
    ObjectList->bat[1].BatSpeed = 6*(FRAMESPEED/ObjectList->FrameRate);

    while(!ObjectList->EventJoined);    //Thread sync

    while(ObjectList->Running){
        CUR = clock();
        TimeDif = CUR - START;

        if(TimeDif >= (1000/ObjectList->FrameRate)){
            START = clock();

            //Game Code
            if(ObjectList->Ball1.Body.x <= 0){
                DirX = 1;
                ObjectList->Score[1]++;

                ObjectList->Ball1.Body={ObjectList->bat[0].Body.x + ObjectList->Ball1.Body.w,ObjectList->bat[0].Body.y + ObjectList->bat[0].Body.h/2,20,20};

                printf("%d:%d  __%f\n",ObjectList->Score[0],ObjectList->Score[1],SpeedX);
                SpeedX = FRAMESPEED/ObjectList->FrameRate + (SpeedX-FRAMESPEED/ObjectList->FrameRate)/2;
                SpeedY = FRAMESPEED/ObjectList->FrameRate + (SpeedX-FRAMESPEED/ObjectList->FrameRate)/2;
            }
            else if(ObjectList->Ball1.Body.x + ObjectList->Ball1.Body.w >= ObjectList->Screen1->Width){
                DirX = -1;
                ObjectList->Score[0]++;

                ObjectList->Ball1.Body={ObjectList->bat[1].Body.x - ObjectList->Ball1.Body.w,ObjectList->bat[1].Body.y + ObjectList->bat[1].Body.h/2,20,20};

                printf("%d:%d\n",ObjectList->Score[0],ObjectList->Score[1]);
                SpeedX = FRAMESPEED/ObjectList->FrameRate + (SpeedX-FRAMESPEED/ObjectList->FrameRate)/2;
                SpeedY = FRAMESPEED/ObjectList->FrameRate + (SpeedX-FRAMESPEED/ObjectList->FrameRate)/2;
            }

            if(SDL_HasIntersectionF(&ObjectList->bat[0].Body,&ObjectList->Ball1.Body) == SDL_TRUE){
                SpeedX += 10.0/ObjectList->FrameRate;
                SpeedY += 8.0/ObjectList->FrameRate;
                DirX = 1;
            }
            else if(SDL_HasIntersectionF(&ObjectList->bat[1].Body,&ObjectList->Ball1.Body) == SDL_TRUE){
                SpeedX += 10.0/ObjectList->FrameRate;
                SpeedY += 8.0/ObjectList->FrameRate;
                DirX = -1;
            }

            if(ObjectList->Ball1.Body.y < 0){
                DirY = 1;
            }
            else if(ObjectList->Ball1.Body.y + ObjectList->Ball1.Body.h >= ObjectList->Screen1->Height){
                DirY = -1;
            }

            //Player Bat Movement
            if(state[SDL_SCANCODE_W]){
                ObjectList->bat[0].Move(0, -ObjectList->bat[0].BatSpeed);

                if (ObjectList->bat[0].Body.y < 0){
                    ObjectList->bat[0].Body.y = 0;
                }
            }
            if(state[SDL_SCANCODE_S]){
                ObjectList->bat[0].Move(0, ObjectList->bat[0].BatSpeed);

                if(ObjectList->bat[0].Body.y > ObjectList->Screen1->Height - ObjectList->bat[0].Body.h){
                    ObjectList->bat[0].Body.y = ObjectList->Screen1->Height - ObjectList->bat[0].Body.h;
                }
            }

            //Comp Bat Movement
            ObjectList->bat[1].CompController(ObjectList->Screen1->Width,ObjectList->Screen1->Height,ObjectList->Ball1.Body);
            ObjectList->bat[0].CompController(ObjectList->Screen1->Width,ObjectList->Screen1->Height,ObjectList->Ball1.Body);

            ObjectList->Ball1.Move(SpeedX*DirX*3,SpeedY*DirY);

            //End of Game Code
        }
    }

    printf("Exit EventThread\n");
    pthread_exit(NULL);
    return NULL;
}

//Screen Event Thread
void * EventThread(void * PassedStruct){
    printf("Entered EventThread\n");

    static Screen MainWindow;                           //initialize Mainwindow object
    SDL_Event event;                                    //event stack

    Objects * ObjectList = (Objects *)(PassedStruct);   //Translating passed void * to Object *

    ObjectList->Screen1 = &MainWindow;                  //passing Mainwindow address to global address struct

    if(!MainWindow.init()){
        ObjectList->EventJoined = true;
        ObjectList->Running = false;
        return NULL;
    }

    ObjectList->EventJoined = true;
    ObjectList->Running = true;

    while(ObjectList->Running){
        while(SDL_PollEvent(&event)){
            if (SDL_QUIT == event.type){
                ObjectList->Running = false;
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

    Objects ObjectList_M;
    clock_t START, CUR;

    int TimeDif;
    int Err;

    printf("Assigned\n");

    Err = pthread_create(&ObjectList_M.threads[1], NULL, GameThread, (void *)(&ObjectList_M));
    if (Err != 0) {
        cout << "Error:unable to create Control thread" << endl;

        ObjectList_M.EventJoined = true;
        ObjectList_M.Running = false;
    }

    Err = pthread_create(&ObjectList_M.threads[0], NULL, EventThread, (void *)(&ObjectList_M));
    if (Err != 0) {
        cout << "Error:unable to create Event thread" << endl;

        ObjectList_M.EventJoined = true;
        ObjectList_M.Running = false;
    }
    printf("Thread Made\n");

    while(!ObjectList_M.EventJoined);   //Thread sync
    if(!ObjectList_M.Running){
        return 0;
    }

    SDL_Rect RenderBox={
        0,0,
        ObjectList_M.Screen1->Width, ObjectList_M.Screen1->Height
    };

    ObjectList_M.Ball1.Body={(float)ObjectList_M.Screen1->ScreenX[0]/2,0,20,20};

    SDL_SetRenderDrawColor(ObjectList_M.Screen1->render,0,0,0,255);
    SDL_RenderClear(ObjectList_M.Screen1->render);
    SDL_RenderPresent(ObjectList_M.Screen1->render);

    ObjectList_M.bat[1].Body.x = ObjectList_M.Screen1->ScreenX[0] - ObjectList_M.bat[1].Body.w-10;

    START = clock();
    while(ObjectList_M.Running){       

        CUR = clock();
        TimeDif = CUR - START;

        if(TimeDif >= (1000/ObjectList_M.RefreshRate)){
            START = clock();

            ObjectList_M.Screen1->ResetRender();

            //printf("Ran");
            SDL_SetRenderDrawColor(ObjectList_M.Screen1->render,0,0,0,255);
            SDL_RenderClear(ObjectList_M.Screen1->render);

            ObjectList_M.Ball1.Render(ObjectList_M.Screen1->render,255,255,255);
            ObjectList_M.bat[0].Render(ObjectList_M.Screen1->render,0,0,255);
            ObjectList_M.bat[1].Render(ObjectList_M.Screen1->render,255,0,0);

            SDL_SetRenderDrawColor(ObjectList_M.Screen1->render,255,255,255,255);
            SDL_RenderDrawRect(ObjectList_M.Screen1->render, &RenderBox);

            SDL_RenderPresent(ObjectList_M.Screen1->render);    //Push to window
        }
    }

    //EXIT
    printf("Closing\n");
    ObjectList_M.Screen1->exit();
    pthread_join(ObjectList_M.threads[1],NULL);
    pthread_join(ObjectList_M.threads[0],NULL);
    return EXIT_SUCCESS;

}