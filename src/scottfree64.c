/*
 *  ScottFree-64 - a reworking of ScottFree Revision 1.14b to run on a c64
 *  (C) 2020 - Mark Seelye / mseelye@yahoo.com
 *  Version 0.9
 *
 *  Requires: cc65 dev environment, build essentials (make)
 *  Optional: 
 *     tmpx for assembling the basic bootstrap.
 *     Vice tooling for c1541
 *
 *  build with:
 *    make clean all
 *
 *  Run with:
 *    Your c64! (Or VICE)
 *    Once you load the binary with 
        load "scottfree64",8,1
 *    You use cc65's argument passing like:
 *      run:rem -d myfavgame.dat mysavegame
 *    The BASIC stub has information on how to load run it.
 *    Hint:Cursor up to the load statement, press enter.
 *         After it loads, change the name of the .dat file for the run and press enter.
 *
 *  Notes:
 *      EVERYTHING LOADS VERY SLOWLY - THIS IS JUST A PORT, NO OPTIMIZATION
 *      My goal here was to create a working version with as few changes to the 
 *      SCOTT.C source as possible.
 *      I removed some code that is not used, and removed the -t TRS formatting 
 *      option.
 *      I added some code to RESTART the game because reloading the while cc65 
 *         is super painful, use -r to enable!
 *      I hacked up the top/bottom display to get the room desc appearing more
 *         consistently.
 *      I did not try to optimize this code, it's fairly nested and complex.
 *      I may do my own "Scott Adams" parser and runner, but we'll see how 
 *      this does first though.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *  Original copyright and license for ScottFree Revision 1.14b follows:
 */
/*
 *	ScottFree Revision 1.14b
 *
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 *
 *	You must have an ANSI C compiler to build this program.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <conio.h>
#include <time.h>
#include <unistd.h>

#include "scottfree.h"

/* not used
#define TRUE 1
#define FALSE 0
*/

Header GameHeader;
/* Tail GameTail; not used in this version */
Item *Items;
Room *Rooms;
char **Verbs;
char **Nouns;
char **Messages;
Action *Actions;
int LightRefill;
char NounText[16];
int Counters[16];    /* Range unknown */
int CurrentCounter;
int SavedRoom;
int RoomSaved[16];    /* Range unknown */
/* int DisplayUp; not used in the version */
/* void *Top,*Bottom; not used in this version */
int Redraw;        /* Update item window */
int Restart;        /* mseelye - Flag to restart game, see NewGame() */
char *SavedGame;    /* mseelye - Name of the saved game that was loaded, if any */
int Options;        /* Option flags set */
int Width;        /* Terminal width */
int TopHeight;        /* Height of top window */
int BottomHeight;    /* Height of bottom window */
int InitialPlayerRoom; /* mseelye - added to help with restarting w/o reloading see: NewGame() */

#define RESTORE_SAVED_ON_RESTART 32 /* new option that autoloads saved game on restart */
/* not used in this version
#define TOPCOL    COLOR_MAGENTA   Should be Brown
#define BOTCOL    COLOR_BLUE
*/

#define MyLoc    (GameHeader.PlayerRoom)

long BitFlags=0;    /* Might be >32 flags - I haven't seen >32 yet */

/* mseelye - Added this to allow program to exit woth 0 when done or 1 when error.*/
void Close(int code)
{
    exit(code);
}

/* mseelye - modified this to output message twice in case ncurses issue
 *           put in a getch(); to have it pause before exiting.
 */
void Fatal(char *x)
{
    printf("%s\n",x);
    getchar();
    Close(1);
}

void Aborted()
{
    Fatal("User exit");
}

void ClearScreen(void)
{
    clrscr();
    gotoxy(0,0);
}

void ClearTop(void)
{
    int ct;
    
    for(ct=0;ct<TopHeight;ct++)
    {
        cclearxy (0, ct, Width);
    }
}

void *MemAlloc(int size)
{
    void *t=(void *)malloc(size);
    if(t==NULL) 
    {
        printf("malloc(%d) failed: ", size);
        Fatal("Out of memory");
    }
    return(t);
}

int RandomPercent(int n)
{
    unsigned int rv=rand()<<6;
    rv%=100;
    if(rv<n)
        return(1);
    return(0);
}

int CountCarried()
{
    int ct=0;
    int n=0;
    while(ct<=GameHeader.NumItems)
    {
        if(Items[ct].Location==CARRIED)
            n++;
        ct++;
    }
    return(n);
}

char *MapSynonym(char *word)
{
    int n=1;
    char *tp;
    static char lastword[16];    /* Last non synonym */
    while(n<=GameHeader.NumWords)
    {
        tp=Nouns[n];
        if(*tp=='*')
            tp++;
        else
            strcpy(lastword,tp);
        if(strncasecmp(word,tp,GameHeader.WordLength)==0)
            return(lastword);
        n++;
    }
    return(NULL);
}

int MatchUpItem(char *text, int loc)
{
    char *word=MapSynonym(text);
    int ct=0;
    
    if(word==NULL)
        word=text;
    
    while(ct<=GameHeader.NumItems)
    {
        if(Items[ct].AutoGet && Items[ct].Location==loc &&
            strncasecmp(Items[ct].AutoGet,word,GameHeader.WordLength)==0)
            return(ct);
        ct++;
    }
    return(-1);
}

/* ASCII to PETSCII
 *   If ASCII and the chr$ is 65 to 90 then add 32.
 *   If ASCII and the chr$ is 97 to 122 then subtract 32.
 *   If ASCII and the chr$ is 8 then change it to 20.
 */
int a2p(int c)
{
    if(c>=65 && c<=90)
        return(c+32);
    else if(c>=97 && c<=123)
        return(c-32);
    return(c);
}

char* ReadString (FILE* f, int ca2p)
{
    char tmp[1024];
    char* t;
    int c,nc;
    int ct=0;

    do
    {
        c=fgetc(f);
    }
    while(c!=EOF && isspace(c));
    if(c!='"')
    {
        Fatal("Initial quote expected");
    }
    do
    {
        c=fgetc(f);
        if(c==EOF)
            Fatal("EOF in string");
        if(c=='"')
        {
            nc=fgetc(f);
            if(nc!='"')
            {
                ungetc(nc,f);
                break;
            }
        }
        if(c==0x60) 
            c='"'; /* pdd */
        if(c=='\n')
        {
            tmp[ct++]=c;
            c='\r';    /* 1.12a PC - pdd */
        }
        if(ca2p==1) {
            c=a2p(c); /* Convert to PETSCII */
        }
        tmp[ct++]=c;

        __asm__ ("inc $d020");
    }
    while(1);
    tmp[ct]=0;
    t=MemAlloc(ct+1);
    memcpy(t,tmp,ct+1);
    return(t);
}

void LoadDatabase (FILE* f, int loud)
{
    int ni,na,nw,nr,mc,pr,tr,wl,lt,mn,trm;
    int ct;
    short lo;
    Action *ap;
    Room *rp;
    Item *ip;
/* Load the header */

    if(fscanf(f,"%*d %d %d %d %d %d %d %d %d %d %d %d",
        &ni,&na,&nw,&nr,&mc,&pr,&tr,&wl,&lt,&mn,&trm,&ct)<10)
        Fatal("Invalid database(bad header)");
    GameHeader.NumItems=ni;
    Items=(Item *)MemAlloc(sizeof(Item)*(ni+1));
    GameHeader.NumActions=na;
    Actions=(Action *)MemAlloc(sizeof(Action)*(na+1));
    GameHeader.NumWords=nw;
    GameHeader.WordLength=wl;
    Verbs=(char **)MemAlloc(sizeof(char *)*(nw+1));
    Nouns=(char **)MemAlloc(sizeof(char *)*(nw+1));
    GameHeader.NumRooms=nr;
    Rooms=(Room *)MemAlloc(sizeof(Room)*(nr+1));
    GameHeader.MaxCarry=mc;
    GameHeader.PlayerRoom=pr;
    InitialPlayerRoom=pr;   // mseelye - added this to help with restarting adventure w/o reloading.
    GameHeader.Treasures=tr;
    GameHeader.LightTime=lt;
    LightRefill=lt;
    GameHeader.NumMessages=mn;
    Messages=(char **)MemAlloc(sizeof(char *)*(mn+1));
    GameHeader.TreasureRoom=trm;

/* Load the actions */

    ct=0;
    ap=Actions;
    if(loud)
        printf("\nReading %d actions.",na);
    while(ct<na+1)
    {
        if(fscanf(f,"%hd %hd %hd %hd %hd %hd %hd %hd",
            &ap->Vocab,
            &ap->Condition[0],
            &ap->Condition[1],
            &ap->Condition[2],
            &ap->Condition[3],
            &ap->Condition[4],
            &ap->Action[0],
            &ap->Action[1])!=8)
        {
            printf("Bad action line (%d)\n",ct);
            exit(1);
        }
        __asm__ ("inc $d020");
        if(loud)
            printf(".");
        ap++;
        ct++;
    }
    ct=0;
    if(loud)
        printf("\nReading %d word pairs.",nw);
    while(ct<nw+1)
    {
        Verbs[ct]=ReadString(f,0);
        Nouns[ct]=ReadString(f,0);
        ct++;
        if(loud)
            printf(".");
    }
    ct=0;
    rp=Rooms;
    if(loud)
        printf("\nReading %d rooms.",nr);
    while(ct<nr+1)
    {
        fscanf(f,"%hd %hd %hd %hd %hd %hd",
            &rp->Exits[0],&rp->Exits[1],&rp->Exits[2],
            &rp->Exits[3],&rp->Exits[4],&rp->Exits[5]);
        rp->Text=ReadString(f,1);
        ct++;
        rp++;
        if(loud)
            printf(".");
    }
    ct=0;
    if(loud)
        printf("\nReading %d messages.",mn);
    while(ct<mn+1)
    {
        Messages[ct]=ReadString(f,1);
        ct++;
        if(loud)
            printf(".");
    }
    ct=0;
    if(loud)
        printf("\nReading %d items.",ni);
    ip=Items;
    while(ct<ni+1)
    {
        ip->Text=ReadString(f,0);
        ip->AutoGet=strchr(ip->Text,'/');
        /* Some games use // to mean no auto get/drop word! */
        if(ip->AutoGet && strcmp(ip->AutoGet,"//") && strcmp(ip->AutoGet,"/*"))
        {
            char *t;
            *ip->AutoGet++=0;
            t=strchr(ip->AutoGet,'/');
            if(t!=NULL)
                *t=0;
        }
        fscanf(f,"%hd",&lo);
        ip->Location=(unsigned char)lo;
        ip->InitialLoc=ip->Location;
        ip++;
        ct++;
        if(loud)
            printf(".");
    }
    ct=0;
    /* Discard Comment Strings */
    while(ct<na+1)
    {
        free(ReadString(f,0));
        ct++;
        if(loud)
            printf(".");
    }
    fscanf(f,"%d",&ct);
    if(loud)
        printf("Version %d.%02d of Adventure ",
        ct/100,ct%100);
    fscanf(f,"%d",&ct);
    if(loud)
        printf("%d.\nLoad Complete.\n\n",ct);
}

int OutputPos=0;

void clreol() {
    int x, y;
    
    x=wherex();
    y=wherey();
    cclearxy (x, y, Width-x);
    gotoxy(x,y);
}

void OutReset()
{
    OutputPos=0;
    gotoxy(0,wherey()); /* not 1 */
    clreol();
}

void OutBuf(char *buffer)
{
    char word[80];
    int wp;

    while(*buffer)
    {
        if(OutputPos==0)
        {
            while(*buffer && isspace(*buffer))
            {
                if(*buffer=='\n')
                {
                    /* For last line entry mode: gotoxy(1,BottomHeight); */
                    printf("\n\r");
                    /* For last line entry mode: (1, wherey(), Width); */
                    OutputPos=0;
                }
                buffer++;
            }
        }
        if(*buffer==0)
            return;
        wp=0;
        while(*buffer && !isspace(*buffer))
        {
            word[wp++]=*buffer++;
        }
        word[wp]=0;
/*        fprintf(stderr,"Word '%s' at %d\n",word,OutputPos);*/
        if(OutputPos+strlen(word)>(Width-2))
        {
            /* For last line entry mode: gotoxy(1, BottomHeight); */
            printf("\n\r");
            /* For last line entry mode: cclearxy (1, wherey(), Width); */
            OutputPos=0;
        }
        else
            gotoxy(OutputPos+0, wherey());
        printf("%s", word);
        OutputPos+=strlen(word);
        if(*buffer==0)
            return;

        if(*buffer=='\n' || *buffer=='\r')
        {
            /* For last line entry mode: gotoxy(1,BottomHeight);*/
            printf("\n\r");
            /* For last line entry mode: cclearxy (1, wherey(), Width); */
            OutputPos=0;
        }
        else
        {
            OutputPos++;
            if(OutputPos<(Width-1))
                printf(" ");
        }
        buffer++;
    }
}

/* output petscii instead of raw ascii, s: 0-OutBuf, 1-printf */
void OutputPetscii(char* a, int s)
{
    char buf[512];
    int ct;

    strcpy(buf, a);
    ct = 0;
    do
    {
        if(a[ct]==0 || ct >= 512)
            break;
        buf[ct] = a2p(buf[ct]);
        ct++;
    }
    while(1);
    if(s==0)
        OutBuf(buf);
    else
        printf(buf);
}

void Output (char* a)
{
    char block[512];
    strcpy(block, a);
    OutBuf(block);
}

void OutputNumber(int a)
{
    char buf[16];
    sprintf(buf,"%d",a);
    OutBuf(buf);
}


void Look(int cs)
{
    static char *ExitNames[6]=
    {
        "North","South","East","West","Up","Down"
    };
    Room *r;
    int ct,f;
    int pos;
    int xp,yp;

    /* if clear screen is set, save x/y cursor for later, then clear */
    if(cs == 1) {
        xp = wherex();
        yp = wherey();
        if(yp < TopHeight +1)
            yp = TopHeight +1;
        ClearTop();
        gotoxy(0,1); /* output 1 down as the c64 will move it up one */
    }

    if((BitFlags&(1L<<DARKBIT)) && Items[LIGHT_SOURCE].Location!= CARRIED
            && Items[LIGHT_SOURCE].Location!= MyLoc)
    {
        if(Options&YOUARE)
            printf("You can't see. It is too dark!\n\r");
        else
            printf("I can't see. It is too dark!\n\r");
        gotoxy(xp,yp);  /* put the cursor back */
        return;
    }
    r=&Rooms[MyLoc];
    if(*r->Text=='*')
        printf("%s\n\r",r->Text+1);
    else
    {
        if(Options&YOUARE)
            printf("You are %s\n\r",r->Text);
        else
            printf("I'm in a %s\n\r",r->Text);
    }
    ct=0;
    f=0;
    printf("\n\rObvious exits: ");
    while(ct<6)
    {
        if(r->Exits[ct]!=0)
        {
            if(f==0)
                f=1;
            else
                printf(", ");
            printf("%s",ExitNames[ct]);
        }
        ct++;
    }
    if(f==0)
        printf("none");
    printf(".\n\r");
    ct=0;
    f=0;
    pos=0;
    while(ct<=GameHeader.NumItems)
    {
        if(Items[ct].Location==MyLoc)
        {
            if(f==0)
            {
                if(Options&YOUARE)
                    printf("\n\rYou can also see: ");
                else
                    printf("\n\rI can also see: ");
                pos=16;
                f++;
            } 
            else 
            {
                printf(" - ");
                pos+=3;
            }
            if(pos+strlen(Items[ct].Text)>(Width-5)) /* was 10 */
            {
                pos=0;
                printf("\n\r");
            }
            OutputPetscii(Items[ct].Text, 1); /* PETSCII, use printf */
            pos += strlen(Items[ct].Text);
        }
        ct++;
    }
    clreol();
    printf("\n\r");
    clreol();

    /* Put cursor back */
    if(cs == 1)
        gotoxy(xp,yp);
}

int WhichWord(char *word, char **list)
{
    int n=1;
    int ne=1;
    char *tp;

    while(ne<=GameHeader.NumWords)
    {
        tp=list[ne];
        if(*tp=='*')
            tp++;
        else
            n=ne;
            
        if(strncasecmp(word,tp,GameHeader.WordLength)==0)
            return(n);
        ne++;
    }
    return(-1);
}

void LineInput(char *buf)
{
    int pos=0;
    int ch;
    
    while(1)
    {
        ch=(int)cgetc();
        switch(ch)
        {
            case 10:;
            case 13:;
                buf[pos]=0;
                printf("\n\r");
                return;
            case 8:;
            case 20:; /* added c64 DEL */
            case 127:;
                if(pos>0)
                {
                    printf("\010");
                    pos--;
                }
                break;
            default:
                if(ch>=' '&&ch<=126)
                {
                    buf[pos++]=ch;
                    printf("%c",(char)ch);
                }
                break;
        }
    }
}

void GetInput(int* vb,int* no)
{
    char buf[256];
    char verb[10],noun[10];
    int vc=0,nc=0;
    int num=0;
    do
    {
        do
        {
            Output("\nTell me what to do ? ");
            /* hack: refresh the top here after 
             * the c64 has moved everything around, ignores Redraw for now */
            Look(1);
            LineInput(buf);
            OutReset();
            num=sscanf(buf,"%9s %9s",verb,noun);
        }
        while(num<=0||*buf=='\n'); /* was num==0, sscanf returning -1 on none found? */
        if(num==1)
            *noun=0;
        if(*noun==0 && strlen(verb)==1)
        {
            switch(isupper(*verb)?tolower(*verb):*verb)
            {
                case 'n':strcpy(verb,"NORTH");break;
                case 'e':strcpy(verb,"EAST");break;
                case 's':strcpy(verb,"SOUTH");break;
                case 'w':strcpy(verb,"WEST");break;
                case 'u':strcpy(verb,"UP");break;
                case 'd':strcpy(verb,"DOWN");break;
                /* Brian Howarth interpreter also supports this */
                case 'i':strcpy(verb,"INVENTORY");break;
            }
        }
        nc=WhichWord(verb,Nouns);
        /* The Scott Adams system has a hack to avoid typing 'go' */
        if(nc>=1 && nc <=6)
        {
            vc=1;
        }
        else
        {
            vc=WhichWord(verb,Verbs);
            nc=WhichWord(noun,Nouns);
        }
        *vb = vc;
        *no = nc;
        if(vc==-1)
        {
            Output("\"");
            Output(verb);
            Output("\" is a word I don't know...sorry!\n");
        }
    }
    while(vc==-1);
    strcpy(NounText,noun);    /* Needed by GET/DROP hack */
}

void SaveGame()
{
    char buf[256];
    int ct;
    FILE *f;
    Output("Filename: ");
    LineInput(buf);
    Output("\n");
    f=fopen(buf,"w");
    if(f==NULL)
    {
        Output("Unable to create save file.\n");
        return;
    }
    for(ct=0;ct<16;ct++)
    {
        fprintf(f,"%d %d\n",Counters[ct],RoomSaved[ct]);
    }
    fprintf(f,"%ld %d %hd %d %d %hd\n",BitFlags, (BitFlags&(1L<<DARKBIT))?1:0,
        MyLoc,CurrentCounter,SavedRoom,GameHeader.LightTime);
    for(ct=0;ct<=GameHeader.NumItems;ct++)
        fprintf(f,"%hd\n",(short)Items[ct].Location);
    fclose(f);

    SavedGame = buf;

    Output("Saved.\n");
}

void LoadGame(char *name)
{
    FILE *f=fopen(name,"r");
    int ct=0;
    short lo;
    /* short DarkFlag; */
    int DarkFlag; /* mseelye - fscanf complains when reading mix of int and short*/

    SavedGame = name;
    printf("\nLoading saved game '%s'..", SavedGame);

    if(f==NULL)
    {
        Output("Unable to restore game.");
        return;
    }
    for(ct=0;ct<16;ct++)
    {
        fscanf(f,"%d %d\n",&Counters[ct],&RoomSaved[ct]);
    }
    fscanf(f,"%ld %d %hd %d %d %hd\n",
        &BitFlags,&DarkFlag,&MyLoc,&CurrentCounter,&SavedRoom,
        &GameHeader.LightTime);
    /* Backward compatibility */
    if(DarkFlag)
        BitFlags|=(1L<<15);
    for(ct=0;ct<=GameHeader.NumItems;ct++)
    {
        fscanf(f,"%hd\n",&lo);
        Items[ct].Location=(unsigned char)lo;
    }
    fclose(f);
}

/* This restores state to the initial state so the 
 * player can play again without reloading.
 * Only needed to add InitialPlayerRoom.
 */
void FreshGame()
{
    int ct;

    printf("\nCleaning up..");

    /* clean up counters and room saved */
    ct=0;
    for(ct=0;ct<16;ct++)
    {
        Counters[ct]=0;
        RoomSaved[ct]=0;
    }

    BitFlags = 0;
    GameHeader.PlayerRoom = InitialPlayerRoom;
    CurrentCounter = 0;
    SavedRoom = 0;
    GameHeader.LightTime = LightRefill;
    ct=0;
    while(ct<GameHeader.NumItems+1)
    {
        Items[ct].Location=Items[ct].InitialLoc;
        ct++;
    }
}

/* This determines if we should load a saved game again
 * or start a fresh game again.
 * Option -r disables loading saved game
 */
void NewGame()
{
    ClearScreen();

    if(SavedGame != NULL && (Options&RESTORE_SAVED_ON_RESTART)) {
        LoadGame(SavedGame);
    } else {
        FreshGame();
    }
}

int PerformLine(int ct)
{
    int continuation=0;
    int param[5],pptr=0;
    int act[4];
    int cc=0;
    while(cc<5)
    {
        int cv,dv;
        cv=Actions[ct].Condition[cc];
        dv=cv/20;
        cv%=20;
        switch(cv)
        {
            case 0:
                param[pptr++]=dv;
                break;
            case 1:
                if(Items[dv].Location!=CARRIED)
                    return(0);
                break;
            case 2:
                if(Items[dv].Location!=MyLoc)
                    return(0);
                break;
            case 3:
                if(Items[dv].Location!=CARRIED&&
                    Items[dv].Location!=MyLoc)
                    return(0);
                break;
            case 4:
                if(MyLoc!=dv)
                    return(0);
                break;
            case 5:
                if(Items[dv].Location==MyLoc)
                    return(0);
                break;
            case 6:
                if(Items[dv].Location==CARRIED)
                    return(0);
                break;
            case 7:
                if(MyLoc==dv)
                    return(0);
                break;
            case 8:
                if((BitFlags&(1L<<dv))==0)
                    return(0);
                break;
            case 9:
                if(BitFlags&(1L<<dv))
                    return(0);
                break;
            case 10:
                if(CountCarried()==0)
                    return(0);
                break;
            case 11:
                if(CountCarried())
                    return(0);
                break;
            case 12:
                if(Items[dv].Location==CARRIED||Items[dv].Location==MyLoc)
                    return(0);
                break;
            case 13:
                if(Items[dv].Location==0)
                    return(0);
                break;
            case 14:
                if(Items[dv].Location)
                    return(0);
                break;
            case 15:
                if(CurrentCounter>dv)
                    return(0);
                break;
            case 16:
                if(CurrentCounter<=dv)
                    return(0);
                break;
            case 17:
                if(Items[dv].Location!=Items[dv].InitialLoc)
                    return(0);
                break;
            case 18:
                if(Items[dv].Location==Items[dv].InitialLoc)
                    return(0);
                break;
            case 19:/* Only seen in Brian Howarth games so far */
                if(CurrentCounter!=dv)
                    return(0);
                break;
        }
        cc++;
    }
    /* Actions */
    act[0]=Actions[ct].Action[0];
    act[2]=Actions[ct].Action[1];
    act[1]=act[0]%150;
    act[3]=act[2]%150;
    act[0]/=150;
    act[2]/=150;
    cc=0;
    pptr=0;
    while(cc<4)
    {
        if(act[cc]>=1 && act[cc]<52)
        {
            Output(Messages[act[cc]]);
            Output("\n");
        }
        else if(act[cc]>101)
        {
            Output(Messages[act[cc]-50]);
            Output("\n");
        }
        else switch(act[cc])
        {
            case 0:/* NOP */
                break;
            case 52:
                if(CountCarried()==GameHeader.MaxCarry)
                {
                    if(Options&YOUARE)
                        Output("You are carrying too much.\n");
                    else
                        Output("I've too much to carry!\n");
                    break;
                }
                if(Items[param[pptr]].Location==MyLoc)
                    Redraw=1;
                Items[param[pptr++]].Location= CARRIED;
                break;
            case 53:
                Redraw=1;
                Items[param[pptr++]].Location=MyLoc;
                break;
            case 54:
                Redraw=1;
                MyLoc=param[pptr++];
                break;
            case 55:
                if(Items[param[pptr]].Location==MyLoc)
                    Redraw=1;
                Items[param[pptr++]].Location=0;
                break;
            case 56:
                BitFlags|=1L<<DARKBIT;
                break;
            case 57:
                BitFlags&=~(1L<<DARKBIT);
                break;
            case 58:
                BitFlags|=(1L<<param[pptr++]);
                break;
            case 59:
                if(Items[param[pptr]].Location==MyLoc)
                    Redraw=1;
                Items[param[pptr++]].Location=0;
                break;
            case 60:
                BitFlags&=~(1L<<param[pptr++]);
                break;
            case 61:
                if(Options&YOUARE)
                    Output("You are dead.\n");
                else
                    Output("I am dead.\n");
                BitFlags&=~(1L<<DARKBIT);
                MyLoc=GameHeader.NumRooms;/* It seems to be what the code says! */
                /* Look(0); this just messes stuff up now */
                break;
            case 62:
            {
                /* Bug fix for some systems - before it could get parameters wrong */
                int i=param[pptr++];
                Items[i].Location=param[pptr++];
                Redraw=1;
                break;
            }
            case 63:
                /* mseelye - Cleaned this up so it didn't use Fatal for c64 made it restart */
doneit:         Output("The game is now over.\n");
                getchar();
                Restart=1;
                break;
                /* Close(0); cc65 is hard to restart, just loop! */
            case 64:
                Look(1);
                break;
            case 65:
            {
                int ct=0;
                int n=0;
                while(ct<=GameHeader.NumItems)
                {
                    if(Items[ct].Location==GameHeader.TreasureRoom &&
                      *Items[ct].Text=='*')
                        n++;
                    ct++;
                }
                if(Options&YOUARE)
                    Output("You have stored ");
                else
                    Output("I've stored ");
                OutputNumber(n);
                Output(" treasures.  On a scale of 0 to 100, that rates ");
                OutputNumber((n*100)/GameHeader.Treasures);
                Output(".\n");
                if(n==GameHeader.Treasures)
                {
                    Output("Well done.\n");
                    goto doneit;
                }
                break;
            }
            case 66:
            {
                int ct=0;
                int f=0;
                if(Options&YOUARE)
                    Output("You are carrying:\n");
                else
                    Output("I'm carrying:\n");
                while(ct<=GameHeader.NumItems)
                {
                    if(Items[ct].Location==CARRIED)
                    {
                        if(f==1)
                        {
                            Output(" - ");
                        }
                        f=1;
                        OutputPetscii(Items[ct].Text, 0); /* PETSCII, use OutBuf */
                    }
                    ct++;
                }
                if(f==0)
                    Output("Nothing");
                Output(".\n");
                break;
            }
            case 67:
                BitFlags|=(1L<<0);
                break;
            case 68:
                BitFlags&=~(1L<<0);
                break;
            case 69:
                GameHeader.LightTime=LightRefill;
                if(Items[LIGHT_SOURCE].Location==MyLoc)
                    Redraw=1;
                Items[LIGHT_SOURCE].Location=CARRIED;
                BitFlags&=~(1L<<LIGHTOUTBIT);
                break;
            case 70:
                ClearScreen(); /* pdd. */
                /* not needed OutReset(); */
                gotoxy(0, TopHeight+2);
                break;
            case 71:
                SaveGame();
                break;
            case 72:
            {
                int i1=param[pptr++];
                int i2=param[pptr++];
                int t=Items[i1].Location;
                if(t==MyLoc || Items[i2].Location==MyLoc)
                    Redraw=1;
                Items[i1].Location=Items[i2].Location;
                Items[i2].Location=t;
                break;
            }
            case 73:
                continuation=1;
                break;
            case 74:
                if(Items[param[pptr]].Location==MyLoc)
                    Redraw=1;
                Items[param[pptr++]].Location= CARRIED;
                break;
            case 75:
            {
                int i1,i2;
                i1=param[pptr++];
                i2=param[pptr++];
                if(Items[i1].Location==MyLoc)
                    Redraw=1;
                Items[i1].Location=Items[i2].Location;
                if(Items[i2].Location==MyLoc)
                    Redraw=1;
                break;
            }
            case 76:    /* Looking at adventure .. */
                Look(1);
                break;
            case 77:
                if(CurrentCounter>=0)
                    CurrentCounter--;
                break;
            case 78:
                OutputNumber(CurrentCounter);
                break;
            case 79:
                CurrentCounter=param[pptr++];
                break;
            case 80:
            {
                int t=MyLoc;
                MyLoc=SavedRoom;
                SavedRoom=t;
                Redraw=1;
                break;
            }
            case 81:
            {
                /* This is somewhat guessed. Claymorgue always
                   seems to do select counter n, thing, select counter n,
                   but uses one value that always seems to exist. Trying
                   a few options I found this gave sane results on ageing */
                int t=param[pptr++];
                int c1=CurrentCounter;
                CurrentCounter=Counters[t];
                Counters[t]=c1;
                break;
            }
            case 82:
                CurrentCounter+=param[pptr++];
                break;
            case 83:
                CurrentCounter-=param[pptr++];
                if(CurrentCounter< -1)
                    CurrentCounter= -1;
                /* Note: This seems to be needed. I don't yet
                   know if there is a maximum value to limit too */
                break;
            case 84:
                Output(NounText);
                break;
            case 85:
                Output(NounText);
                Output("\n");
                break;
            case 86:
                Output("\n");
                break;
            case 87:
            {
                /* Changed this to swap location<->roomflag[x]
                   not roomflag 0 and x */
                int p=param[pptr++];
                int sr=MyLoc;
                MyLoc=RoomSaved[p];
                RoomSaved[p]=sr;
                Redraw=1;
                break;
            }
            case 88:
                sleep(2);    /* DOC's say 2 seconds. Spectrum times at 1.5 */
                break;
            case 89:
                pptr++;
                /* SAGA draw picture n */
                /* Spectrum Seas of Blood - start combat ? */
                /* Poking this into older spectrum games causes a crash */
                break;
            default:
                fprintf(stderr,"Unknown action %d [Param begins %d %d]\n",
                    act[cc],param[pptr],param[pptr+1]);
                break;
        }
        cc++;
    }
    return(1+continuation);
}


int PerformActions(int vb,int no)
{
    static int disable_sysfunc=0;    /* Recursion lock */
    int d=BitFlags&(1L<<DARKBIT);
    
    int ct=0;
    int fl;
    int doagain=0;
    if(vb==1 && no == -1 )
    {
        Output("Give me a direction too.\n");
        return(0);
    }
    if(vb==1 && no>=1 && no<=6)
    {
        int nl;
        if(Items[LIGHT_SOURCE].Location==MyLoc ||
           Items[LIGHT_SOURCE].Location==CARRIED)
               d=0;
        if(d)
            Output("Dangerous to move in the dark!\n");
        nl=Rooms[MyLoc].Exits[no-1];
        if(nl!=0)
        {
            MyLoc=nl;
            Output("O.K.\n");
            return(0);
        }
        if(d)
        {
            if(Options&YOUARE)
                Output("You fell down and broke your neck.\n");
            else
                Output("I fell down and broke my neck.\n");
            //sleep(5); - just use a getchar() for now;
            getchar();
            Restart=1;
            return(0);
            // exit(0); - mseelye added restart logic to light death
        }
        if(Options&YOUARE)
            Output("You can't go in that direction.\n");
        else
            Output("I can't go in that direction.\n");
        return(0);
    }
    fl= -1;
    while(ct<=GameHeader.NumActions)
    {
        int vv,nv;
        vv=Actions[ct].Vocab;
        /* Think this is now right. If a line we run has an action73
           run all following lines with vocab of 0,0 */
        if(vb!=0 && (doagain&&vv!=0))
            break;
        /* Oops.. added this minor cockup fix 1.11 */
        if(vb!=0 && !doagain && fl== 0)
            break;
        nv=vv%150;
        vv/=150;
        if((vv==vb)||(doagain&&Actions[ct].Vocab==0))
        {
            if((vv==0 && RandomPercent(nv))||doagain||
                (vv!=0 && (nv==no||nv==0)))
            {
                int f2;
                if(fl== -1)
                    fl= -2;
                if((f2=PerformLine(ct))>0)
                {
                    /* ahah finally figured it out ! */
                    fl=0;
                    if(f2==2)
                        doagain=1;
                    if(vb!=0 && doagain==0)
                        return(0);
                }
            }
        }
        ct++;
        if(Actions[ct].Vocab!=0)
            doagain=0;
    }
    if(fl!=0 && disable_sysfunc==0)
    {
        int i;
        if(Items[LIGHT_SOURCE].Location==MyLoc ||
           Items[LIGHT_SOURCE].Location==CARRIED)
               d=0;
        if(vb==10 || vb==18) /* TAKE || DROP */
        {
            /* Yes they really _are_ hardcoded values */
            if(vb==10) /* TAKE */
            {
                if(strcasecmp(NounText,"ALL")==0)
                {
                    int ct=0;
                    int f=0;
                    
                    if(d)
                    {
                        Output("It is dark.\n");
                        return 0;
                    }
                    while(ct<=GameHeader.NumItems)
                    {
                        if(Items[ct].Location==MyLoc && Items[ct].AutoGet!=NULL && Items[ct].AutoGet[0]!='*')
                        {
                            no=WhichWord(Items[ct].AutoGet,Nouns);
                            disable_sysfunc=1;    /* Don't recurse into auto get ! */
                            PerformActions(vb,no);    /* Recursively check each items table code */
                            disable_sysfunc=0;
                            if(CountCarried()==GameHeader.MaxCarry)
                            {
                                if(Options&YOUARE)
                                    Output("You are carrying too much.\n");
                                else
                                    Output("I've too much to carry.\n");
                                return(0);
                            }
                            Items[ct].Location= CARRIED;
                            Redraw=1;
                            OutputPetscii(Items[ct].Text, 0); /* PETSCII, use OutBuf  */
                            Output(": O.K.\n");
                            f=1;
                        }
                        ct++;
                    }
                    if(f==0)
                        Output("Nothing taken.\n");
                    return(0);
                }
                if(no==-1)
                {
                    Output("What ?\n");
                    return(0);
                }
                if(CountCarried()==GameHeader.MaxCarry)
                {
                    if(Options&YOUARE)
                        Output("You are carrying too much.\n");
                    else
                        Output("I've too much to carry.\n");
                    return(0);
                }
                i=MatchUpItem(NounText,MyLoc);
                if(i==-1)
                {
                    if(Options&YOUARE)
                        Output("It is beyond your power to do that.\n");
                    else
                        Output("It's beyond my power to do that.\n");
                    return(0);
                }
                Items[i].Location= CARRIED;
                Output("O.K.\n");
                Redraw=1;
                return(0);
            }
            if(vb==18) /* DROP */
            {
                if(strcasecmp(NounText,"ALL")==0)
                {
                    int ct=0;
                    int f=0;
                    while(ct<=GameHeader.NumItems)
                    {
                        if(Items[ct].Location==CARRIED && Items[ct].AutoGet && Items[ct].AutoGet[0]!='*')
                        {
                            no=WhichWord(Items[ct].AutoGet,Nouns);
                            disable_sysfunc=1;
                            PerformActions(vb,no);
                            disable_sysfunc=0;
                            Items[ct].Location=MyLoc;
                            OutputPetscii(Items[ct].Text, 0); /* PETSCII, use OutBuf */
                            Output(": O.K.\n");
                            Redraw=1;
                            f=1;
                        }
                        ct++;
                    }
                    if(f==0)
                        Output("Nothing dropped.\n");
                    return(0);
                }
                if(no==-1)
                {
                    Output("What ?\n");
                    return(0);
                }
                i=MatchUpItem(NounText,CARRIED);
                if(i==-1)
                {
                    if(Options&YOUARE)
                        Output("It's beyond your power to do that.\n");
                    else
                        Output("It's beyond my power to do that.\n");
                    return(0);
                }
                Items[i].Location=MyLoc;
                Output("O.K.\n");
                Redraw=1;
                return(0);
            }
        }
    }
    return(fl);
}

int main(int argc, char *argv[])
{
    FILE *f;
    int vb,no;
    
    while(argv[1])
    {
        if(*argv[1]!='-')
            break;
        switch(argv[1][1])
        {
            case 'y':
                Options|=YOUARE;
                break;
            case 'i':
                Options&=~YOUARE;
                break;
            case 'd':
                Options|=DEBUGGING;
                break;
            case 's':
                Options|=SCOTTLIGHT;
                break;
            case 'p':
                Options|=PREHISTORIC_LAMP;
                break;
            case 'r':
                Options|=RESTORE_SAVED_ON_RESTART;
                break;
            case 'h':
            default:
                fprintf(stderr,"%s: [-h] [-y] [-s] [-i] [-d] [-p] [-r] <gamename> [savedgame].\n\r",argv[0]);
                fprintf(stderr," -h:help\n\r -y:'you are'\n\r -s:scottlight\n\r -i:'i am'\n\r -d:debug\n\r -p:old lamp behavior\n\r -r:restore recent save on restart\n\r gamename:required\n\r savedgame:optional\n\r");
                exit(1);
        }
        if(argv[1][2]!=0)
        {
            fprintf(stderr,"%s: option -%c does not take a parameter.\n\r",
                argv[0],argv[1][1]);
            exit(1);
        }
        argv++;
        argc--;
    }

    if(argc!=2 && argc!=3)
    {
        fprintf(stderr,"run:rem <db file> <save file>\n\r");
        exit(1);
    }
    f=fopen(argv[1],"r");
    if(f==NULL)
    {
        perror(argv[1]);
        exit(1);
    }

    Width = 40; 
    TopHeight = 10;
    BottomHeight = 15;

    /* DisplayUp=1; not used in this version */
    OutReset();
    ClearScreen();

    /* cleaned up for 40 cols */
    printf("ScottFree64 {VERSION}, A c64 port of:\
ScottFree, Scott Adams game driver in C\
Release 1.14b(PC), (c) 1993,1994,1995\
Swansea University Computer Society.\
Distributed under the GNU software\nlicense\n\n");
    LoadDatabase(f,(Options&DEBUGGING)?1:0);
    fclose(f);
    if(argc==3)
        LoadGame(argv[2]);
    srand(time(NULL));

    textcolor (0);
    bgcolor (11);
    bordercolor (11);

    /* Added a loop here, and a Restart flag, and a fucntion to reset the game. */
    while(1)
    {
        ClearScreen(); /* was Look(1) */
        gotoxy(0, TopHeight+1);
        Redraw = 1;

        while(1)
        {
/* using look hack after prompt
            if(Redraw!=0)
            {
                Look(1);
                Redraw=0;
            }
*/
            PerformActions(0,0);
/*
            if(Redraw!=0)
            {
                Look(1);
                Redraw=0;
            }
*/
            if(Restart!=0)
            {
                NewGame();
                Restart=0;
                break;
            }
            else
            {
                GetInput(&vb,&no);
                switch(PerformActions(vb,no))
                {
                    case -1:Output("I don't understand your command.\n");
                        break;
                    case -2:Output("I can't do that yet.\n");
                        break;
                }
                /* Brian Howarth games seem to use -1 for forever */
                if(Items[LIGHT_SOURCE].Location/*==-1*/!=DESTROYED && GameHeader.LightTime!= -1)
                {
                    GameHeader.LightTime--;
                    if(GameHeader.LightTime<1)
                    {
                        BitFlags|=(1L<<LIGHTOUTBIT);
                        if(Items[LIGHT_SOURCE].Location==CARRIED ||
                            Items[LIGHT_SOURCE].Location==MyLoc)
                        {
                            if(Options&SCOTTLIGHT)
                                Output("Light has run out! ");
                            else
                                Output("Your light has run out. ");
                        }
                        if(Options&PREHISTORIC_LAMP)
                            Items[LIGHT_SOURCE].Location=DESTROYED;
                    }
                    else if(GameHeader.LightTime<25)
                    {
                        if(Items[LIGHT_SOURCE].Location==CARRIED ||
                            Items[LIGHT_SOURCE].Location==MyLoc)
                        {
                    
                            if(Options&SCOTTLIGHT)
                            {
                                Output("Light runs out in ");
                                OutputNumber(GameHeader.LightTime);
                                Output(" turns. ");
                            }
                            else
                            {
                                if(GameHeader.LightTime%5==0)
                                    Output("Your light is growing dim. ");
                            }
                        }
                    }
                }
            }
        }
    }
}
