/*
 *  ScottFree-64 - a Reworking of ScottFree Revision 1.14b for the Commodore 64
 *  (C) 2020 - Mark Seelye / mseelye@yahoo.com
 *  Version 2.0.1
 *  Heavier cbm optimizations.
 *
 *  Requires: cc65 dev environment, build essentials (make)
 *  Optional: 
 *     Vice tooling for c1541 (making the disk images), petcat (basic stub)
 *
 *  build with:
 *    make clean all
 *
 *  Run with:
 *    Your Commodore 64! (Or VICE)
 *    Once you load the binary with 
        load "scottfree64",8,1
 *    You use cc65's argument passing like:
 *      run:rem -d myfavgame.dat mysavegame
 *    The BASIC stub has information on how to load run it.
 *    Hint:Cursor up to the load statement, press enter.
 *         After it loads, change the name of the .dat file for the run and press enter.
 *
 *  Notes:
 *      This is a reworking of the reworking (v1), it has heavier optimizations for memory for the c64.
 *      This has all the changes that v1 had, but also removes as much stdio as possible as 
 *      printf, scanf, etc. take a lot of memory.
 *      In addition to output (printf/sprintf), and file i/o (fscanf) I also created 
 *      an all asm input parser for verb and noun that takes much less space than having
 *      sscanf linked in.
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
#include <conio.h>
#include <unistd.h>

#include "scottfree.h"
#include "sf64.h"
#include <stdint.h>

#ifndef PEEK
    #define PEEK(addr)         (*(unsigned char*) (addr))
#endif

Header GameHeader;
Item *Items;
Room *Rooms;
char **Verbs;
char **Nouns;
char **Messages;
Action *Actions;
int LightRefill = 0;
char NounText[16];
int Counters[16];           // Range unknown
int CurrentCounter = 0;
int SavedRoom = 0;
int RoomSaved[16];          // Range unknown
int Redraw = 0;             // Update item window
int Restart = 0;            // Flag to restart game, see NewGame()
char *SavedGame;            // Name of the saved game that was loaded, if any
int Options = 0;            // Option flags set
int Width = 0;              // Terminal width
int TopHeight = 0;          // Height of top window
// Not Used: int BottomHeight;    // Height of bottom window
int InitialPlayerRoom = 0;  // added to help with restarting w/o reloading see: NewGame()
char block[512];            // global buffer, used by multiple functions for temp storage

#define RESTORE_SAVED_ON_RESTART 32 // new option that autoloads saved game on restart

#define MyLoc    (GameHeader.PlayerRoom)

long BitFlags=0;    // Might be >32 flags, previous comment said, "I haven't seen >32 yet"

void Fatal(char *x) {
    print(x);
    print_char('\n');
    cgetc();
    exit(1);
}

// TODO: asm opt
void ClearScreen(void) {
    clrscr();
    gotoxy(0,0);
}

// TODO: asm opt
void ClearTop(void) {
    uint8_t ct=0; // opt //int ct=0;
    
    while(ct < TopHeight) {
        cclearxy (0, ct++, Width);
    }
}

void *MemAlloc(uint16_t size) {
    void *t=(void *)malloc(size);
    if(t==NULL) {
        // printf("malloc(%d) failed: ", size); // opt
        print("malloc failed, size: ");
        print_number(size);
        Fatal("Out of memory");
    }
    return(t);
}

// TODO: This needs to be fixed,
//       it steps % changes 1-4=%4 5-8:%7  9-12:%10 etc
uint8_t RandomPercent(uint16_t n) {
    uint16_t rv=rand()<<6; // wtf is this?
    rv%=100;
    if(rv<n) {
        return(1);
    }
    return(0);
}

// TODO: asm opt
uint8_t CountCarried() {
    uint8_t ct=0;
    uint8_t n=0;
    while(ct<=GameHeader.NumItems)
    {
        if(Items[ct].Location==CARRIED)
            n++;
        ct++;
    }
    return(n);
}

// TODO: asm opt
char *MapSynonym(char *word) {
    uint8_t n=1;
    char *tp;
    static char lastword[16];     // Last non synonym
    while(n<=GameHeader.NumWords) {
        tp=Nouns[n];
        if(*tp=='*') {
            tp++;
        } else {
            strcpy(lastword,tp);  // TODO opt?
        }
        if(strncasecmp(word,tp,GameHeader.WordLength)==0) {
            return(lastword);
        }
        n++;
    }
    return(NULL);
}

// TODO: asm opt
int8_t MatchUpItem(char *text, int loc) {
    char *word=MapSynonym(text);
    uint8_t ct=0; // opt // int ct=0;
    
    if(word==NULL) {
        word=text;
    }
    
    while(ct<=GameHeader.NumItems) {
        if(Items[ct].AutoGet && Items[ct].Location==loc &&
            strncasecmp(Items[ct].AutoGet,word,GameHeader.WordLength)==0) {
            return(ct);
        }
        ct++;
    }
    return(-1);
}

// read file until next whitespace, convert to 32 bit integer
uint32_t cbm_read_next(uint8_t filenum) {
    char buf[32]; // TODO: can this use common block?
    int8_t c = 0;
    uint8_t ct=0;
    int8_t t=0;
    // Skip over initial ws
    do {
        t = cbm_read(filenum, &c, 1);
    } while(t > 0 && isspace((int)c));
    if(t<0) {
        Fatal("Error reading data");
    }
    if(t==0) {
        Fatal("Unexpected EOF while reading data");
    }
    buf[ct++]=c; // first usable character after any whitespace
    do {
        t = cbm_read(filenum, &c, 1);
        if(t <= 0 || c==' ' || c=='\n' || c=='\r') // ws = parse delim c==EOF || 
            break;
        buf[ct++]=c;
    } while(ct<31);
    buf[ct]=0;
    return(atol(buf));
}

// Note: uses common global block buffer, instead of local tmp buffer
char* ReadString (uint8_t filenum, uint8_t ca2p) {
    uint8_t ct=0;
    uint8_t c, nc;
    uint8_t pbc = 0; // pushback buffer, our "ungetch"
    int8_t t = 0;
    char* r; // result

    do {
        if(pbc==0) {
            t = cbm_read(filenum, &c, 1);
        } else {
            c = pbc;
            pbc = 0;
        }
    } while(t > 0 && isspace((int)c));
    if(c!='"') {
        Fatal("Initial quote expected");
    }
    do {
        if(pbc==0) {
            t = cbm_read(filenum, &c, 1);
        } else {
            c = pbc;
            pbc = 0;
        }
        if(t<=0) { // cbm EOF or ERROR
            Fatal("EOF in string");
        }
        if(c=='"') {
            if(pbc==0) {
                t = cbm_read(filenum, &nc, 1);
            } else {
                nc = pbc;
                pbc = 0;
            }
            if(nc!='"') {
                pbc=nc; // "ungetc(nc,f)"
                break;
            }
        }
        if(c==0x60) {
            c='"';
        }
        if(c=='\n') {
            block[ct++]=c;
            c='\r';
        }
        if(ca2p==1) {
            c=a2p(c);
        }
        block[ct++]=c;
    } while(1);
    block[ct]=0;
    r=MemAlloc(ct+1);
    memcpy(r,block,ct+1);
    return(r);
}

void DebugMessage(uint8_t loud, char *msg, uint8_t num) {
    if(loud) {
        print_char('\n');
        print_number(num);
        print_char(' ');
        print(msg);
    }
}

// Load Standard DAT File
void LoadDatabase (char *filename, uint8_t loud) {
    uint8_t ct=0;
    uint8_t filenum=1;
    uint8_t device=8; // cbm // TODO: fix this.. wtf is wrong with $ba
    uint8_t lo;
    Action *ap;
    Room *rp;
    Item *ip;

    // Open File
     if (cbm_open(filenum, device, 2, filename)) {
         Fatal("File error!");
     }

    GameHeader.Unknown=cbm_read_next(filenum);
    GameHeader.NumItems=cbm_read_next(filenum);
    GameHeader.NumActions=cbm_read_next(filenum);
    GameHeader.NumWords=cbm_read_next(filenum);
    GameHeader.NumRooms=cbm_read_next(filenum);
    GameHeader.MaxCarry=cbm_read_next(filenum);
    GameHeader.PlayerRoom=cbm_read_next(filenum);
    GameHeader.Treasures=cbm_read_next(filenum);
    GameHeader.WordLength=cbm_read_next(filenum);
    GameHeader.LightTime=cbm_read_next(filenum);
    GameHeader.NumMessages=cbm_read_next(filenum);
    GameHeader.TreasureRoom=cbm_read_next(filenum);
    InitialPlayerRoom=GameHeader.PlayerRoom;   // mseelye - added this to help with restarting adventure w/o reloading.
    LightRefill=GameHeader.LightTime;
    
    Items=(Item *)MemAlloc(sizeof(Item)*(GameHeader.NumItems+1));
    Actions=(Action *)MemAlloc(sizeof(Action)*(GameHeader.NumActions+1));
    Verbs=(char **)MemAlloc(sizeof(char *)*(GameHeader.NumWords+1));
    Nouns=(char **)MemAlloc(sizeof(char *)*(GameHeader.NumWords+1));
    Rooms=(Room *)MemAlloc(sizeof(Room)*(GameHeader.NumRooms+1));
    Messages=(char **)MemAlloc(sizeof(char *)*(GameHeader.NumMessages+1));

    ct=0;
    ap=Actions;
    DebugMessage(loud, "actions", GameHeader.NumActions+1);
    while(ct < GameHeader.NumActions+1) {
        ap->Vocab=(short)cbm_read_next(filenum);
        ap->Condition[0]=(short)cbm_read_next(filenum);
        ap->Condition[1]=(short)cbm_read_next(filenum);
        ap->Condition[2]=(short)cbm_read_next(filenum);
        ap->Condition[3]=(short)cbm_read_next(filenum);
        ap->Condition[4]=(short)cbm_read_next(filenum);
        ap->Action[0]=(short)cbm_read_next(filenum);
        ap->Action[1]=(short)cbm_read_next(filenum);
        if(loud) {
            print_char('.');
        }
        ap++;
        ct++;
    }

    ct=0;
    DebugMessage(loud, "pairs", GameHeader.NumWords+1);
    while(ct<GameHeader.NumWords+1) {
        Verbs[ct]=ReadString(filenum,0);
        Nouns[ct]=ReadString(filenum,0);
        ct++;
        if(loud) {
            print_char('.');
        }
    }

    ct=0;
    rp=Rooms;
    DebugMessage(loud, "rooms", GameHeader.NumRooms+1);
    while(ct<GameHeader.NumRooms+1) {
        rp->Exits[0]=(short)cbm_read_next(filenum);
        rp->Exits[1]=(short)cbm_read_next(filenum);
        rp->Exits[2]=(short)cbm_read_next(filenum);
        rp->Exits[3]=(short)cbm_read_next(filenum);
        rp->Exits[4]=(short)cbm_read_next(filenum);
        rp->Exits[5]=(short)cbm_read_next(filenum);
        rp->Text=ReadString(filenum,1);
        ct++;
        rp++;
        if(loud) {
            print_char('.');
        }
    }

    ct=0;
    DebugMessage(loud, "messages", GameHeader.NumMessages+1);
    while(ct<GameHeader.NumMessages+1) {
        Messages[ct]=ReadString(filenum,1);
        ct++;
        if(loud) {
            print_char('.');
        }
    }

    ct=0;
    DebugMessage(loud, "items", GameHeader.NumItems+1);
    ip=Items;
    while(ct<GameHeader.NumItems+1) {
        ip->Text=ReadString(filenum,0);
        ip->AutoGet=strchr(ip->Text,'/'); // TODO: opt?
        // Some games use // to mean no auto get/drop word!
        if(ip->AutoGet && strcmp(ip->AutoGet,"//") && strcmp(ip->AutoGet,"/*"))
        {
            char *t;
            *ip->AutoGet++=0;
            t=strchr(ip->AutoGet,'/'); // TODO: opt?
            if(t!=NULL)
                *t=0;
        }
        lo=cbm_read_next(filenum);
        ip->Location=(unsigned char)lo;
        ip->InitialLoc=ip->Location;
        ip++;
        ct++;
        if(loud) {
            print_char('.');
        }
    }
    
    ct=0;
    while(ct<GameHeader.NumActions+1)
    {
        free(ReadString(filenum,0)); // Discard Comment Strings
        ct++;
        if(loud) {
            print_char('.');
        }
    }

    if(loud) {
        uint16_t adv = 0;
        adv=cbm_read_next(filenum);
        print("\nv");
        print_number(adv/100);
        print_char('.');
        print_number(adv%100);
        print(" of Adv. ");
        adv=cbm_read_next(filenum);
        print_number(adv);
        // skip magic number
        //adv=cbm_read_next(filenum);
        print("\nDone!\n");
    }

    cbm_close(filenum);
}

// Load Binary DAT String
uint8_t *LoadString(filenum) {
    uint8_t *string;
    uint8_t length = 0;
    cbm_read(filenum, &length, 1);
    string=(uint8_t *)MemAlloc((length+1) * sizeof(uint8_t));
    if(length == 0) {
        cbm_read(filenum, &length, 1);
        string[0]='\0';
        return(string);
    }
    cbm_read(filenum, string, (length+1) * sizeof(uint8_t));
    return(string);
}

// Load Binary DAT File
void LoadDatabaseBinary (char *filename, uint8_t loud) {
    uint8_t ct=0;
    uint8_t filenum=1;
    uint8_t device=8; // TODO: get current device!
    Action *ap;
    Room *rp;
    Item *ip;

    if (cbm_open(filenum, device, 2, filename)) {
        Fatal("File error!");
    }

    cbm_read(filenum, &GameHeader.Unknown, sizeof(GameHeader));
    InitialPlayerRoom=GameHeader.PlayerRoom;
    LightRefill=GameHeader.LightTime;

    // Allocate this all here, may help with fragmentation
    Actions=(Action *)MemAlloc(sizeof(Action)*(GameHeader.NumActions+1));
    Verbs=(char **)MemAlloc(sizeof(char *)*(GameHeader.NumWords+1));
    Nouns=(char **)MemAlloc(sizeof(char *)*(GameHeader.NumWords+1));
    Rooms=(Room *)MemAlloc(sizeof(Room)*(GameHeader.NumRooms+1));
    Messages=(char **)MemAlloc(sizeof(char *)*(GameHeader.NumMessages+1));
    Items=(Item *)MemAlloc(sizeof(Item)*(GameHeader.NumItems+1));

    ct=0;
    ap=Actions;
    DebugMessage(loud, "actions", GameHeader.NumActions+1);
    while(ct < GameHeader.NumActions+1) {
        cbm_read(filenum, ap, sizeof(Action));
        ct++;
        ap++;
        if(loud) {
            print_char('.');
        }
    }

    ct=0;
    DebugMessage(loud, "pairs", GameHeader.NumWords+1);
    while(ct < GameHeader.NumWords+1) {
        Verbs[ct] = LoadString(filenum);
        Nouns[ct] = LoadString(filenum);
        ct++;
        if(loud) {
            print_char('.');
        }
    }

    ct=0;
    rp=Rooms;
    DebugMessage(loud, "rooms", GameHeader.NumRooms+1);
    while(ct < GameHeader.NumRooms+1) {
        cbm_read(filenum, rp->Exits, 6 * sizeof(uint16_t));
        rp->Text = LoadString(filenum);
        a2p_string(rp->Text, 0);
        ct++;
        rp++;
        if(loud) {
            print_char('.');
        }
    }

    ct=0;
    DebugMessage(loud, "messages", GameHeader.NumMessages+1);
    while(ct < GameHeader.NumMessages+1) {
        Messages[ct]=LoadString(filenum);
        a2p_string(Messages[ct], 0);
        ct++;
        if(loud) {
            print_char('.');
        }
    }

    ct=0;
    ip=Items;
    DebugMessage(loud, "items", GameHeader.NumItems+1);
    while(ct < GameHeader.NumItems+1) {
        ip->Text=LoadString(filenum);
        ip->AutoGet=LoadString(filenum);
        // SF likes Autoget to be NULL not empty
        if (ip->AutoGet[0]==0) {
            free(ip->AutoGet);
            ip->AutoGet = NULL;
        }
        cbm_read(filenum, &ip->Location, 1);
        ip->InitialLoc=ip->Location;
        ct++;
        ip++;
        if(loud) {
            print_char('.');
        }
    }

    /* Discard Comment Strings */
    ct=0;
    while(ct < GameHeader.NumActions+1) {
        free(LoadString(filenum)); // load it and free it
        ct++;
        if(loud) {
            print_char('.');
        }
    }

    if(loud) {
        uint16_t adv = 0;
        cbm_read(filenum, &adv, 2);
        print("\nv");
        print_number(adv/100);
        print_char('.');
        print_number(adv%100);
        print(" of Adv. ");
        cbm_read(filenum, &adv, 2);
        print_number(adv);
        // skip magic number
        //cbm_read(filenum, &ct, 2);
        print("\nDone!\n");
    }
    cbm_close(filenum);
}

uint8_t OutputPos=0; // opt // int OutputPos=0;

// TODO: asm opt
void clreol() {
    uint8_t x, y; // opt // int x, y;
    x=wherex();
    y=wherey();
    cclearxy(x, y, Width-x);
    gotoxy(x,y);
}

void OutReset()
{
    OutputPos=0;
    gotoxy(0,wherey()); // not 1
    clreol();
}

// note: can't use global block buffer, would overlap from Output()
void OutBuf(char *buffer) {
    char word[80]; 
    uint8_t wp;

    while(*buffer) {
        if(OutputPos==0) {
            while(*buffer && isspace(*buffer)) { // TODO: opt for isspace?
                if(*buffer=='\n') {
                    print("\n\r");
                    OutputPos=0;
                }
                buffer++;
            }
        }
        if(*buffer==0) {
            return;
        }
        wp=0;
        while(*buffer && !isspace(*buffer)) { // TODO: opt for isspace?
            word[wp++]=*buffer++;
        }
        word[wp]=0;
        if(OutputPos+strlen(word)>(Width-2)) {
            print("\n\r");
            OutputPos=0;
        } else {
            gotoxy(OutputPos+0, wherey());
        }
        print(word);
        OutputPos+=strlen(word);
        if(*buffer==0) {
            return;
        }
        if(*buffer=='\n' || *buffer=='\r') {
            print("\n\r");
            OutputPos=0;
        } else {
            OutputPos++;
            if(OutputPos<(Width-1)) {
                print(" ");
            }
        }
        buffer++;
    }
}

// output petscii instead of raw ascii, mode: 0-OutBuf, 1-print
// Note: Uses global block buffer
void OutputPetscii(char* text, uint8_t mode) {
    uint8_t ct = 0;

    strcpy(block, text); // TODO: asm opt?
    do {
        if(text[ct]==0) {
            break;
        }
        block[ct] = a2p(block[ct]);
        ct++;
    } while(1);
    if(mode==0) {
        OutBuf(block);
    } else {
        print(block);
    }
}

// Note: Uses global block buffer
void Output (char* text) {
    strcpy(block, text);
    OutBuf(block);
}

void OutputNumber(int num) {
    OutBuf((char *)bufnum32(num)); // Note: doesn't print signed
}

void Look(uint8_t cs) {
    static char *ExitNames[6]= {
        "North","South","East","West","Up","Down"
    };
    Room *r;
    uint8_t ct,f;
    uint8_t pos;
    uint8_t xp,yp;

    // if clear screen is set, save x/y cursor for later, then clear
    if(cs == 1) {
        xp = wherex();
        yp = wherey();
        if(yp < TopHeight +1) {
            yp = TopHeight +1;
        }
        ClearTop();
        gotoxy(0,1); // output 1 down as the c64 will move it up one
    }

    if((BitFlags&(1L<<DARKBIT)) && Items[LIGHT_SOURCE].Location!= CARRIED
            && Items[LIGHT_SOURCE].Location!= MyLoc) {
        if(Options&YOUARE) {
            print("You can't see. It is too dark!\n\r");
        } else {
            print("I can't see. It is too dark!\n\r");
        }
        gotoxy(xp,yp);
        return;
    }
    r=&Rooms[MyLoc];
    if(*r->Text=='*')
    {
        print(r->Text+1);
        print("\n\r");
    }
    else
    {
        if(Options&YOUARE) {
            print("You are ");
            print(r->Text);
            print("\n\r"); 
        } else {
            print("I'm in a ");
            print(r->Text);
            print("\n\r"); 
        }
    }
    ct=0;
    f=0;
    print("\n\rObvious exits: ");
    while(ct<6) {
        if(r->Exits[ct]!=0) {
            if(f==0) {
                f=1;
            } else {
                print(", ");
            }
            print(ExitNames[ct]);
        }
        ct++;
    }
    if(f==0) {
        print("none");
    }
    print(".\n\r");
    ct=0;
    f=0;
    pos=0;
    while(ct<=GameHeader.NumItems) {
        if(Items[ct].Location==MyLoc) {
            if(f==0) {
                if(Options&YOUARE) {
                    print("\n\rYou can also see: ");
                } else {
                    print("\n\rI can also see: ");
                }
                pos=16;
                f++;
            } else {
                print(" - ");
                pos+=3;
            }
            if(pos+strlen(Items[ct].Text)>(Width-5)) { // was 10
                pos=0;
                print("\n\r");
            }
            OutputPetscii(Items[ct].Text, 1);
            pos += strlen(Items[ct].Text);
        }
        ct++;
    }
    clreol();
    print("\n\r");
    clreol();

    if(cs == 1) {
        gotoxy(xp,yp); // put cursor back
    }
}

int8_t WhichWord(char *word, char **list) {
    uint8_t n=1;
    uint8_t ne=1;
    char *tp;

    while(ne<=GameHeader.NumWords) {
        tp=list[ne];
        if(*tp=='*') {
            tp++;
        } else {
            n=ne;
        }
        if(strncasecmp(word,tp,GameHeader.WordLength)==0) {
            return(n);
        }
        ne++;
    }
    return(-1);
}

// Change text, border/bg colors on the C64
// TODO: opt asm?
void ColorChange(uint8_t code) {
    uint8_t curT = 0;
    uint8_t curD = 0;
    uint8_t curB = 0;
    int8_t sh = 0;
    curT = textcolor(0); // get Current Text Color
    curD = PEEK(0xd020); // get Current Border Color
    curB = PEEK(0xd021); // get current BG Color
    sh = (code >= 0x89)?-1:1; // Shift pressed = -1, no shift = 1

    switch (code) {
        case 0x85: // F1
        case 0x89: // F2
            curT += sh;
            break;
        case 0x86: // F3
        case 0x8a: // F4
            curD += sh;
            break;
        case 0x87: // F5
        case 0x8b: // F6
            curB += sh;
            break;
        case 0x88: // F7
            curT = 0; // Black
            curD = 11; // Grey
            curB = 11; // Still Grey
            break;
        case 0x8c: // F8
            curT = rand();
            curD = rand();
            curB = rand();
            break;
    }
    textcolor(curT & 15);
    bordercolor (curD & 15);
    bgcolor(curB & 15);
    Look(1);
}

// TODO opt as asm?
void LineInput(char *buf, uint8_t max) {
    uint8_t pos=0;
    uint8_t ch;
    
    while(1) {
        ch=(uint8_t)cgetc();
        if(pos >= max-1) {
            ch=13; // auto enter at max
        }
        switch(ch) {
            case 10:;
            case 13:;
                buf[pos]=0;
                print("\n\r");
                return;
            case 8:;
            case 20:; // c64 DEL
            case 127:;
                if(pos>0) {
                    print("\010");
                    pos--;
                }
                break;
            default:
                if(ch >= 0x85 && ch <= 0x8c) { // Fkeys
                     ColorChange(ch);
                }
                if(ch>=' '&&ch<=126) {
                    buf[pos++]=ch;
                    print_char(ch);
                }
                break;
        }
    }
}

// Note: Can't use global block buffer, also reduced to 80 characters
void GetInput(int8_t* vb, int8_t* no) {
    char buf[80];
    char verb[10],noun[10];
    int8_t vc=0,nc=0;
    int8_t num=0;
    do {
        do {
            Output("\nTell me what to do ? ");
            Look(1); // hack: refresh the top here after c64 scroll, etc.
            LineInput(buf, 80);
            OutReset();
            num=parseVerbNoun(buf, 9, verb, noun); // "sscanf"
        } while(num<=0||*buf=='\n');
        if(num==1) {
            *noun=0;
        }
        if(*noun==0 && strlen(verb)==1) {
            switch(isupper(*verb)?tolower(*verb):*verb) { // TODO: opt isupper tolower? or just expand switch?
                case 'n':strcpy(verb,"NORTH");break;
                case 'e':strcpy(verb,"EAST");break;
                case 's':strcpy(verb,"SOUTH");break;
                case 'w':strcpy(verb,"WEST");break;
                case 'u':strcpy(verb,"UP");break;
                case 'd':strcpy(verb,"DOWN");break;
                case 'i':strcpy(verb,"INVENTORY");break; // Brian Howarth interpreter also supports i
            }
        }
        nc=WhichWord(verb,Nouns);
        /* The Scott Adams system has a hack to avoid typing 'go' */
        if(nc>=1 && nc <=6) {
            vc=1;
        } else {
            vc=WhichWord(verb,Verbs);
            nc=WhichWord(noun,Nouns);
        }
        *vb = vc;
        *no = nc;
        if(vc==-1) {
            Output("\"");
            Output(verb);
            Output("\" is a word I don't know...sorry!\n");
        }
    } while(vc==-1);
    strcpy(NounText,noun); // Needed by GET/DROP hack
}

void cbm_write_value(uint8_t filenum, uint32_t value, char *end) {
    char *num;
    num = (char *)bufnum32(value);
    cbm_write(filenum, (char *)num, num[11]); // hack buffer has size at + 11
    cbm_write(filenum, end, 1);
}

// Note: Can't use global block buffer, used to save last saved game file name
void SaveGame() {
    // Note: static variables maintain values, sloppy cheat to keep saved game name
    char filename[32];
    uint8_t ct=0;
    uint8_t filenum=1;
    uint8_t device=8; // TODO: current device?
    char *sp=" ";
    char *cr="\n";
    Output("Filename: ");
    LineInput(filename, 32);
    Output("\n");
    if (cbm_open(filenum, device, 1, filename)) {
        Output("Unable to create save file.\n");
        return;
    }
    while(ct < 16) {
        cbm_write_value(filenum, Counters[ct], sp);
        cbm_write_value(filenum, RoomSaved[ct], cr);
        ct++;
    }
    cbm_write_value(filenum, BitFlags, sp);
    cbm_write_value(filenum, ((BitFlags&(1L<<DARKBIT))?1:0), sp);
    cbm_write_value(filenum, MyLoc, sp);
    cbm_write_value(filenum, CurrentCounter, sp);
    cbm_write_value(filenum, SavedRoom, sp);
    if(GameHeader.LightTime < 0) {
        cbm_write(filenum, "-1\n", 3);
    } else {
        cbm_write_value(filenum, GameHeader.LightTime, cr);
    }
    
    ct=0;
    while(ct <= GameHeader.NumItems) {
        cbm_write_value(filenum, Items[ct].Location, cr);
        ct++;
    }
    cbm_close(filenum);

    SavedGame = filename;

    Output("Saved.\n");
}

void LoadGame(char *name) {
    uint8_t ct=0;
    uint8_t filenum=1;
    uint8_t device=8; // TODO: current device?
    uint8_t DarkFlag;

    SavedGame = name;
    print("\nLoading saved game '");
    print(SavedGame);
    print("'..");

     if (cbm_open(filenum, device, 2, name)) {
         Output("Unable to restore game.\n");
         return;
     }

    while(ct < 16) {
        Counters[ct] = cbm_read_next(filenum);
        RoomSaved[ct] = cbm_read_next(filenum);
        ct++;
    }
    BitFlags = cbm_read_next(filenum);
    DarkFlag = cbm_read_next(filenum);
    MyLoc = cbm_read_next(filenum);
    CurrentCounter = cbm_read_next(filenum);
    SavedRoom = cbm_read_next(filenum);
    GameHeader.LightTime = (int8_t)cbm_read_next(filenum);

    /* Backward compatibility */
    if(DarkFlag) {
        BitFlags|=(1L<<15);
    }
    ct=0;
    while(ct <= GameHeader.NumItems) {
        Items[ct].Location=(unsigned char)cbm_read_next(filenum);;
        ct++;
    }
    cbm_close(filenum);
}

// This restores state to the initial state so the 
// player can play again without reloading.
// Only needed to add InitialPlayerRoom.
void FreshGame() {
    uint8_t ct=0;

    // Clean up counters and room saved
    while(ct < 16) {
        Counters[ct]=0;
        RoomSaved[ct]=0;
        ct++;
    }

    BitFlags = 0;
    GameHeader.PlayerRoom = InitialPlayerRoom;
    CurrentCounter = 0;
    SavedRoom = 0;
    GameHeader.LightTime = LightRefill;
    ct=0;
    while(ct < GameHeader.NumItems+1) {
        Items[ct].Location=Items[ct].InitialLoc;
        ct++;
    }
}

// This determines if we should load a saved game again
// or start a fresh game again.
// Option -r enabled loading saved game
void NewGame() {
    ClearScreen();

    if(SavedGame != NULL && (Options&RESTORE_SAVED_ON_RESTART)) {
        LoadGame(SavedGame);
    } else {
        FreshGame();
    }
}

// TODO: asm opt
uint8_t PerformLine(uint8_t ct) {
    uint8_t continuation=0;
    uint8_t pptr=0;
    uint16_t cc=0;
    uint16_t act[4];
    uint8_t param[5];
    while(cc<5) {
        uint16_t cv,dv;
        cv=Actions[ct].Condition[cc];
        dv=cv/20;
        cv%=20;
        switch(cv) {
            case 0:
                param[pptr++]=dv;
                break;
            case 1:
                if(Items[dv].Location!=CARRIED) {
                    return(0);
                }
                break;
            case 2:
                if(Items[dv].Location!=MyLoc) {
                    return(0);
                }
                break;
            case 3:
                if(Items[dv].Location!=CARRIED&&
                    Items[dv].Location!=MyLoc) {
                    return(0);
                }
                break;
            case 4:
                if(MyLoc!=dv) {
                    return(0);
                }
                break;
            case 5:
                if(Items[dv].Location==MyLoc) {
                    return(0);
                }
                break;
            case 6:
                if(Items[dv].Location==CARRIED) {
                    return(0);
                }
                break;
            case 7:
                if(MyLoc==dv) {
                    return(0);
                }
                break;
            case 8:
                if((BitFlags&(1L<<dv))==0) {
                    return(0);
                }
                break;
            case 9:
                if(BitFlags&(1L<<dv)) {
                    return(0);
                }
                break;
            case 10:
                if(CountCarried()==0) {
                    return(0);
                }
                break;
            case 11:
                if(CountCarried()) {
                    return(0);
                }
                break;
            case 12:
                if(Items[dv].Location==CARRIED||Items[dv].Location==MyLoc) {
                    return(0);
                }
                break;
            case 13:
                if(Items[dv].Location==0) {
                    return(0);
                }
                break;
            case 14:
                if(Items[dv].Location) {
                    return(0);
                }
                break;
            case 15:
                if(CurrentCounter>dv) {
                    return(0);
                }
                break;
            case 16:
                if(CurrentCounter<=dv) {
                    return(0);
                }
                break;
            case 17:
                if(Items[dv].Location!=Items[dv].InitialLoc) {
                    return(0);
                }
                break;
            case 18:
                if(Items[dv].Location==Items[dv].InitialLoc) {
                    return(0);
                }
                break;
            case 19:// Only seen in Brian Howarth games so far
                if(CurrentCounter!=dv) {
                    return(0);
                }
                break;
        }
        cc++;
    }
    // Actions
    act[0]=Actions[ct].Action[0];
    act[2]=Actions[ct].Action[1];
    act[1]=act[0]%150;
    act[3]=act[2]%150;
    act[0]/=150;
    act[2]/=150;
    cc=0;
    pptr=0;
    while(cc<4) {
        if(act[cc]>=1 && act[cc]<52) {
            Output(Messages[act[cc]]);
            Output("\n");
        } else if(act[cc]>101) {
            Output(Messages[act[cc]-50]);
            Output("\n");
        } else switch(act[cc]) {
            case 0:// NOP
                break;
            case 52:
                if(CountCarried()==GameHeader.MaxCarry) {
                    if(Options&YOUARE) {
                        Output("You are carrying too much.\n");
                    } else {
                        Output("I've too much to carry!\n");
                    }
                    break;
                }
                if(Items[param[pptr]].Location==MyLoc) {
                    Redraw=1;
                }
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
                if(Items[param[pptr]].Location==MyLoc) {
                    Redraw=1;
                }
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
                if(Items[param[pptr]].Location==MyLoc) {
                    Redraw=1;
                }
                Items[param[pptr++]].Location=0;
                break;
            case 60:
                BitFlags&=~(1L<<param[pptr++]);
                break;
            case 61:
                if(Options&YOUARE) {
                    Output("You are dead.\n");
                } else {
                    Output("I am dead.\n");
                }
                BitFlags&=~(1L<<DARKBIT);
                // Comment from sf1.14: It seems to be what the code says! */
                MyLoc=GameHeader.NumRooms;
                break;
            case 62: {
                // Bug fix for some systems - before it could get parameters wrong
                uint8_t i=param[pptr++];
                Items[i].Location=param[pptr++];
                Redraw=1;
                break;
            }
            case 63:
                // Note: death/game over
                Output("The game is now over.\n");
                cgetc();
                Restart=1; // just restart
                break;
            case 64:
                Look(1);
                break;
            case 65: {
                // Note: death/game over
                uint8_t ct=0;
                uint8_t n=0;
                while(ct<=GameHeader.NumItems) {
                    if(Items[ct].Location==GameHeader.TreasureRoom &&
                      *Items[ct].Text=='*') {
                        n++;
                    }
                    ct++;
                }
                if(Options&YOUARE) {
                    Output("You have stored ");
                } else {
                    Output("I've stored ");
                }
                OutputNumber(n);
                Output(" treasures.  On a scale of 0 to 100, that rates ");
                OutputNumber((n*100)/GameHeader.Treasures);
                Output(".\n");
                if(n==GameHeader.Treasures) {
                    Output("Well done.\nThe game is now over.\n");
                    cgetc();
                    Restart=1; // just restart
                }
                break;
            }
            case 66: {
                uint8_t ct=0;
                uint8_t f=0;
                if(Options&YOUARE) {
                    Output("You are carrying:\n");
                } else {
                    Output("I'm carrying:\n");
                }
                while(ct<=GameHeader.NumItems) {
                    if(Items[ct].Location==CARRIED) {
                        if(f==1) {
                            Output(" - ");
                        }
                        f=1;
                        OutputPetscii(Items[ct].Text, 0); /* PETSCII, use OutBuf */
                    }
                    ct++;
                }
                if(f==0) {
                    Output("Nothing");
                }
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
                if(Items[LIGHT_SOURCE].Location==MyLoc) {
                    Redraw=1;
                }
                Items[LIGHT_SOURCE].Location=CARRIED;
                BitFlags&=~(1L<<LIGHTOUTBIT);
                break;
            case 70:
                ClearScreen();
                gotoxy(0, TopHeight+2);
                break;
            case 71:
                SaveGame();
                break;
            case 72: {
                uint8_t i1=param[pptr++];
                uint8_t i2=param[pptr++];
                uint8_t t=Items[i1].Location;
                if(t==MyLoc || Items[i2].Location==MyLoc) {
                    Redraw=1;
                }
                Items[i1].Location=Items[i2].Location;
                Items[i2].Location=t;
                break;
            }
            case 73:
                continuation=1;
                break;
            case 74:
                if(Items[param[pptr]].Location==MyLoc) {
                    Redraw=1;
                }
                Items[param[pptr++]].Location= CARRIED;
                break;
            case 75: {
                uint8_t i1,i2;
                i1=param[pptr++];
                i2=param[pptr++];
                if(Items[i1].Location==MyLoc) {
                    Redraw=1;
                }
                Items[i1].Location=Items[i2].Location;
                if(Items[i2].Location==MyLoc) {
                    Redraw=1;
                }
                break;
            }
            case 76:    // Looking at adventure ..
                Look(1);
                break;
            case 77:
                if(CurrentCounter>=0) {
                    CurrentCounter--;
                }
                break;
            case 78:
                OutputNumber(CurrentCounter);
                break;
            case 79:
                CurrentCounter=param[pptr++];
                break;
            case 80: {
                uint8_t t=MyLoc;
                MyLoc=SavedRoom;
                SavedRoom=t;
                Redraw=1;
                break;
            }
            case 81: {
                /* This is somewhat guessed. Claymorgue always
                   seems to do select counter n, thing, select counter n,
                   but uses one value that always seems to exist. Trying
                   a few options I found this gave sane results on ageing */
                uint8_t t=param[pptr++];
                uint16_t c1=CurrentCounter;
                CurrentCounter=Counters[t];
                Counters[t]=c1;
                break;
            }
            case 82:
                CurrentCounter+=param[pptr++];
                break;
            case 83:
                CurrentCounter-=param[pptr++];
                if(CurrentCounter< -1) {
                    CurrentCounter= -1;
                }
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
            case 87: {
                /* Changed this to swap location<->roomflag[x]
                   not roomflag 0 and x */
                uint8_t p=param[pptr++];
                uint8_t sr=MyLoc;
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
                // note: normally to error channel
                print("Unknown action ");
                print_number(act[cc]);
                print(" [Param begins ");
                print_number(param[pptr]);
                print_char(' ');
                print_number(param[pptr+1]);
                print("]\n");
                break;
        }
        cc++;
    }
    return(1+continuation);
}


int8_t PerformActions(int8_t vb,int8_t no) {
    static uint8_t disable_sysfunc=0;
    uint8_t d=BitFlags&(1L<<DARKBIT);
    
    uint8_t ct=0; // opt // int ct=0;
    int8_t fl; // opt // int fl;
    uint8_t doagain=0; // opt // int doagain=0;
    if(vb==1 && no == -1 ) {
        Output("Give me a direction too.\n");
        return(0);
    }
    if(vb==1 && no>=1 && no<=6) {
        uint8_t nl; // opt // int nl;
        if(Items[LIGHT_SOURCE].Location==MyLoc ||
           Items[LIGHT_SOURCE].Location==CARRIED) {
               d=0;
        }
        if(d) {
            Output("Dangerous to move in the dark!\n");
        }
        nl=Rooms[MyLoc].Exits[no-1];
        if(nl!=0) {
            MyLoc=nl;
            Output("O.K.\n");
            return(0);
        }
        if(d) {
            // Note: death
            if(Options&YOUARE) {
                Output("You fell down and broke your neck.\n");
            } else {
                Output("I fell down and broke my neck.\n");
            }
            cgetc();
            Restart=1;
            return(0);
        }
        if(Options&YOUARE) {
            Output("You can't go in that direction.\n");
        } else {
            Output("I can't go in that direction.\n");
        }
        return(0);
    }
    fl= -1;
    while(ct<=GameHeader.NumActions) {
        uint16_t vv,nv;
        vv=Actions[ct].Vocab;
        /* Think this is now right. If a line we run has an action73
           run all following lines with vocab of 0,0 */
        if(vb!=0 && (doagain&&vv!=0)) {
            break;
        }
        /* Oops.. added this minor cockup fix 1.11 */
        if(vb!=0 && !doagain && fl== 0) {
            break;
        }
        nv=vv%150;
        vv/=150;
        if((vv==vb)||(doagain&&Actions[ct].Vocab==0)) {
            if((vv==0 && RandomPercent(nv))||doagain||
                (vv!=0 && (nv==no||nv==0))) {
                uint8_t f2; // opt // int f2;
                if(fl== -1) {
                    fl= -2;
                }
                if((f2=PerformLine(ct))>0) {
                    /* ahah finally figured it out ! */
                    fl=0;
                    if(f2==2) {
                        doagain=1;
                    }
                    if(vb!=0 && doagain==0) {
                        return(0);
                    }
                }
            }
        }
        ct++;
        if(Actions[ct].Vocab!=0) {
            doagain=0;
        }
    }
    if(fl!=0 && disable_sysfunc==0) {
        int8_t i;
        if(Items[LIGHT_SOURCE].Location==MyLoc ||
           Items[LIGHT_SOURCE].Location==CARRIED) {
               d=0;
        }
        if(vb==10 || vb==18) { // TAKE || DROP
            /* Yes they really _are_ hardcoded values */
            if(vb==10) { // TAKE
                if(strcasecmp(NounText,"ALL")==0) {
                    uint8_t ct=0;
                    uint8_t f=0;
                    
                    if(d) {
                        Output("It is dark.\n");
                        return 0;
                    }
                    while(ct<=GameHeader.NumItems) {
                        if(Items[ct].Location==MyLoc && Items[ct].AutoGet!=NULL && Items[ct].AutoGet[0]!='*') {
                            no=WhichWord(Items[ct].AutoGet,Nouns);
                            disable_sysfunc=1;    /* Don't recurse into auto get ! */
                            PerformActions(vb,no);    /* Recursively check each items table code */
                            disable_sysfunc=0;
                            if(CountCarried()==GameHeader.MaxCarry) {
                                if(Options&YOUARE) {
                                    Output("You are carrying too much.\n");
                                } else {
                                    Output("I've too much to carry.\n");
                                }
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
                    if(f==0) {
                        Output("Nothing taken.\n");
                    }
                    return(0);
                }
                if(no==-1) {
                    Output("What ?\n");
                    return(0);
                }
                if(CountCarried()==GameHeader.MaxCarry) {
                    if(Options&YOUARE) {
                        Output("You are carrying too much.\n");
                    } else {
                        Output("I've too much to carry.\n");
                    }
                    return(0);
                }
                i=MatchUpItem(NounText,MyLoc);
                if(i==-1) {
                    if(Options&YOUARE) {
                        Output("It is beyond your power to do that.\n");
                    } else {
                        Output("It's beyond my power to do that.\n");
                    }
                    return(0);
                }
                Items[i].Location= CARRIED;
                Output("O.K.\n");
                Redraw=1;
                return(0);
            }
            if(vb==18) { // DROP
                if(strcasecmp(NounText,"ALL")==0) {
                    uint8_t ct=0;
                    uint8_t f=0;
                    while(ct<=GameHeader.NumItems) {
                        if(Items[ct].Location==CARRIED && Items[ct].AutoGet && Items[ct].AutoGet[0]!='*') {
                            no=WhichWord(Items[ct].AutoGet,Nouns);
                            disable_sysfunc=1;
                            PerformActions(vb,no);
                            disable_sysfunc=0;
                            Items[ct].Location=MyLoc;
                            OutputPetscii(Items[ct].Text, 0); //  PETSCII, use OutBuf
                            Output(": O.K.\n");
                            Redraw=1;
                            f=1;
                        }
                        ct++;
                    }
                    if(f==0) {
                        Output("Nothing dropped.\n");
                    }
                    return(0);
                }
                if(no==-1) {
                    Output("What ?\n");
                    return(0);
                }
                i=MatchUpItem(NounText,CARRIED);
                if(i==-1) {
                    if(Options&YOUARE) {
                        Output("It's beyond your power to do that.\n");
                    } else {
                        Output("It's beyond my power to do that.\n");
                    }
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

void CheckLight() {
    // Brian Howarth games seem to use -1 for forever
    if(Items[LIGHT_SOURCE].Location/*==-1*/!=DESTROYED && GameHeader.LightTime!= -1) {
        GameHeader.LightTime--;
        if(GameHeader.LightTime<1) {
            BitFlags|=(1L<<LIGHTOUTBIT);
            if(Items[LIGHT_SOURCE].Location==CARRIED ||
                Items[LIGHT_SOURCE].Location==MyLoc) {
                if(Options&SCOTTLIGHT) {
                    Output("Light has run out! ");
                } else {
                    Output("Your light has run out. ");
                }
            }
            if(Options&PREHISTORIC_LAMP) {
                Items[LIGHT_SOURCE].Location=DESTROYED;
            }
        } else if(GameHeader.LightTime<25) {
            if(Items[LIGHT_SOURCE].Location==CARRIED ||
                Items[LIGHT_SOURCE].Location==MyLoc) {
        
                if(Options&SCOTTLIGHT) {
                    Output("Light runs out in ");
                    OutputNumber(GameHeader.LightTime);
                    Output(" turns. ");
                } else {
                    if(GameHeader.LightTime%5==0) {
                        Output("Your light is growing dim. ");
                    }
                }
            }
        }
    }
}

// Main Game Loop here
void GameLoop() {
    int8_t vb=0,no=0;

    while(1) {
        // Note: Redraws removed, using look hack after prompt
        PerformActions(0,0);
        if(Restart!=0) {
            Restart=0;
            return; // Returns, Outer loop
        } else {
            GetInput(&vb,&no);
            switch(PerformActions(vb,no)) {
                case -1:
                    Output("I don't understand your command.\n");
                    break;
                case -2:
                    Output("I can't do that yet.\n");
                    break;
            }
            CheckLight();
        }
    }
}

// Parse arg to Options, or fail
uint8_t ParseArgs(uint8_t argc, char *argv[]) {
    uint8_t argo = 0;
    while(argv[1]) {
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
                print(argv[0]);
                print(": [-h] [-y] [-s] [-i] [-d] [-p] [-r] <gamename> [savedgame].\n\r");
                print(" -h:help\n\r -y:'you are'\n\r -s:scottlight\n\r -i:'i am'\n\r -d:debug\n\r -p:old lamp behavior\n\r -r:restore recent save on restart\n\r gamename:required\n\r savedgame:optional\n\r");
                exit(1);
        }
        if(argv[1][2]!=0) {
            // opt // fprintf(stderr,"%s: option -%c does not take a parameter.\n\r", argv[0],argv[1][1]);
            print(argv[0]);
            print(": option -");
            print_char(argv[1][1]);
            print("does not take a parameter.\n\r");
            exit(1);
        }
        argv++;
        argc--;
        argo++;
    }

    if(argc!=2 && argc!=3) {
        print("run:rem <db file> <save file>\n\r");
        exit(1);
    }
    
    return(argo);
}

void GameSetup(char *filename, char *savedgame) {
    uint8_t filenum = 1;
    uint8_t device = 8; // TODO: Current Device
    uint16_t chk[3] = {0,0,0};

    // TODO: device
    if (cbm_open(filenum, device, 2, filename)) {
        print("Unable to load \"");
        print(filename);
        print_char('\"');
        exit(1);
    }

    Width = 40; 
    TopHeight = 10;

    OutReset();
    ClearScreen();
    textcolor (14);     // Light blue
    bgcolor (0);        // Black
    bordercolor (11);   // Dark Grey

    print("ScottFree64 {VERSION}, A c64 port of:\
ScottFree, Scott Adams game driver in C\
Release 1.14b(PC), (c) 1993,1994,1995\
Swansea University Computer Society.\
Distributed under the GNU software\nlicense\n\n");

    print("MemFree(");
    print_number(_heapmemavail());
    print(")\nLoading ");
    print(filename);
    print(" as ");

    // Detect ascii or binary
    // Close and reopen in LoadDatabase call
    chk[0] = cbm_read_next(filenum);
    chk[1] = cbm_read_next(filenum);
    chk[2] = cbm_read_next(filenum);
    cbm_close(filenum);
    
    // if could not read text numbers, should be binary DAT
    if(chk[0]==0 && chk[1]==0 && chk[2]==0) { 
        print("BDAT\n");
        LoadDatabaseBinary(filename,(Options&DEBUGGING)?1:0); // load Binary DAT file
    } else {
        print("DAT\n");
        LoadDatabase(filename,(Options&DEBUGGING)?1:0);  // load DAT file
    }
    
    // Load Saved Game
    if(savedgame!=NULL) {
        LoadGame(savedgame);
    }
    
    // seed from c64 $a2 and $d012
    srand((PEEK(0x00a2) << 8) | (PEEK(0xd012)));

    print("MemFree(");
    print_number(_heapmemavail());
    print(")\n");

    // Initial Colors
    textcolor (0);    // black
    bgcolor (11);     // grey
    bordercolor (11); // grey
}

uint8_t main(uint8_t argc, char *argv[]) {
    int8_t argo = 0;
    argo = ParseArgs(argc, argv);
    
    GameSetup(argv[argo+1], (argc-argo==3)? argv[argo+2] : NULL);

    // Outer Loop
    // Loops here, restart flag used to reset/restart the game.
    while(1) {
        ClearScreen();
        gotoxy(0, TopHeight+1);
        Redraw = 1;
        GameLoop(); // Loops until Restart flag set;
        NewGame();
    }
    return(0);
}
